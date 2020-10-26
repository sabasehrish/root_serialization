#include "HDFOutputer.h"
#include "summarize_serializers.h"
#include "lz4.h"
#include <iostream>
#include <cstring>
#include <set>

using namespace cce::tf;
using namespace HighFive;
using product_t = std::vector<char>; 

void HDFOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& s = serializers_[iLaneIndex];
  s.reserve(iDPs.size());
  for(auto const& dp: iDPs) {
    s.emplace_back(dp.name(), dp.classType());
  }
  products_.reserve(100);
}

void HDFOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  auto& laneSerializers = serializers_[iLaneIndex];
  auto group = iCallback.group();
  laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
}

void HDFOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto tempBuffer = std::make_unique<std::vector<std::vector<char>>>(writeDataProductsToOutputBuffer(serializers_[iLaneIndex]));
  queue_.push(*iCallback.group(), [this, iEventID, iLaneIndex, callback=std::move(iCallback), buffer=std::move(tempBuffer)]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      const_cast<HDFOutputer*>(this)->output(iEventID, serializers_[iLaneIndex],*buffer);
      buffer.reset();
        serialTime_ += std::chrono::duration_cast<decltype(serialTime_)>(std::chrono::high_resolution_clock::now() - start);
      callback.doneWaiting();
    });
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    parallelTime_ += time.count();
}

void HDFOutputer::printSummary() const  {
  std::cout <<"HDFOutputer\n  total serial time at end event: "<<serialTime_.count()<<"us\n"
    "  total parallel time at end event: "<<parallelTime_.load()<<"us\n";
  summarize_serializers(serializers_);
}


using product_t = std::vector<char>;
std::vector<product_t> 
get_prods(std::vector<product_t> const & input, 
         int prod_index, 
         int stride) {
  std::vector<product_t> tmp; 
  for(int j = prod_index; j< input.size(); j+=stride) {
    tmp.push_back(input[j]);
  }
  return tmp;
}

product_t
flatten(std::vector<product_t> const& tmp) {
  std::vector<char> tmp1 = tmp[0];
  for (int i = 1; i<tmp.size(); ++i)
     tmp1.insert(end(tmp1), begin(tmp[i]), end(tmp[i]));
  return tmp1;
}

std::vector<size_t>
get_sizes(std::vector<product_t> const & prods) {
  std::vector<size_t> sizes;
  for(auto p: prods) 
    sizes.push_back(p.size());
  return sizes;
}

void HDFOutputer::output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers, std::vector<std::vector<char>>const& iBuffer) {
  //std::cout << iEventID.event << ", " << iBuffer.size() << '\n';
  if(firstTime_) {
    writeFileHeader(iSerializers);
    firstTime_ = false;
  } 
    std::cout << "Writing data products " << iEventID.event << '\n';
    for(auto & s: dataProductIndices_) {
      DataSet dataset = file_.getDataSet(s.first);
      auto prods = get_prods(iBuffer, s.second, dataProductIndices_.size());
      std::vector<char> tmp = flatten(prods);
      dataset.resize({dataset.getDimensions()[0]+tmp.size()});
      dataset.write(tmp); 
      auto sizes = get_sizes(prods);
      auto s1 = s.first+"_sz";
      DataSet dataset2 = file_.getDataSet(s1);
      dataset2.resize({dataset2.getDimensions()[0]+sizes.size()});
      dataset2.write(sizes);
    }
}

void HDFOutputer::writeFileHeader(std::vector<SerializerWrapper> const& iSerializers) {
  int i = 0;
  for(auto const& w: iSerializers) {
    std::string s{w.name()};
   // products_.push_back(w.blob());
    dataProductIndices_.push_back({s, i});
    ++i;
    std::string s_sz = s+"_sz";
    if(!file_.exist(s) && !file_.exist(s_sz)) {
      DataSetCreateProps props;
      props.add(Chunking(std::vector<hsize_t>{10}));
      DataSpace dataspace = DataSpace({0}, {DataSpace::UNLIMITED});
      DataSet dataset = file_.createDataSet<char>(s, dataspace, props);
      DataSet dataset_sz = file_.createDataSet<int>(s_sz, dataspace, props); 
    } 
  }
}


std::vector<std::vector<char>> HDFOutputer::writeDataProductsToOutputBuffer(std::vector<SerializerWrapper> const& iSerializers) const{
  std::vector<std::vector<char>> cBuffer;
  std::vector<std::string> prod_names;
  for(auto& s: iSerializers) {
    //   std::cout<<"   " << s.name() << " size " << std::to_string(s.blob().size()) << "\n";
       auto prod_sizes = s.blob().size();
       std::string prod_name{s.name()};
       prod_names.push_back(prod_name);
       cBuffer.push_back(s.blob());
    }
  return cBuffer;
}
