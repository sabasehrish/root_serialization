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
  products_.reserve(10000);
  events_.reserve(100);
}

void HDFOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  auto& laneSerializers = serializers_[iLaneIndex];
  auto group = iCallback.group();
  laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
}

void HDFOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  queue_.push(*iCallback.group(), [this, iEventID, iLaneIndex, callback=std::move(iCallback)]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      const_cast<HDFOutputer*>(this)->output(iEventID, serializers_[iLaneIndex]);
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

std::pair<product_t, std::vector<size_t>> 
HDFOutputer::
get_prods_and_sizes(std::vector<product_t> & input, 
         int prod_index, 
         int stride) {
  product_t products; 
  std::vector<size_t> sizes; 
  for(int j = prod_index; j< input.size(); j+=stride) {
    //sizes.push_back(offsets_+=input[j].size());
    sizes.push_back(input[j].size());
    products.insert(end(products), std::make_move_iterator(begin(input[j])), std::make_move_iterator(end(input[j])));
  }
  return {products, sizes};
}

template < typename T>
void write_ds(Group g, std::string name, std::vector<T> data) {
    DataSet ds = g.getDataSet(name);
    auto old_dim = ds.getDimensions()[0];
    auto new_dim = old_dim + data.size();
    ds.resize({new_dim});
    ds.select({old_dim}, {data.size()}).write(data);
}

void HDFOutputer::output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) {
  if(firstTime_) {
    writeFileHeader(iEventID, iSerializers);
    firstTime_ = false;
  }
  // accumulate events before writing, go through all the data products in the curret event
//  else 
  {
    for(auto& s: iSerializers) 
       products_.push_back(s.blob());
    events_.push_back(iEventID.event);
    ++batch_;
  }
  if (batch_ == 2) { //max_batch_size){
    auto g = file_.getGroup("Lumi");
    write_ds<int>(g, "Event_IDs", events_);
    auto const dpi_size = dataProductIndices_.size();
    for(auto & [name, index]: dataProductIndices_) {
      auto [prods, sizes] = get_prods_and_sizes(products_, index, dpi_size);
      write_ds<char>(g, name, prods);
      write_ds<size_t>(g, name+"_sz", sizes);
    }
    batch_ = 0;
    products_.clear();
    events_.clear();
  }
}
void HDFOutputer::writeFileHeader(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) {
  DataSetCreateProps props;
  props.add(Chunking(std::vector<hsize_t>{128}));
  //props.add(Shuffle());
  //props.add(Deflate(9));
  DataSpace dataspace = DataSpace({0}, {DataSpace::UNLIMITED});
  if(!file_.exist("Lumi")) {
    file_.createGroup("Lumi");
  } 
  auto g = file_.getGroup("Lumi");
  if(!g.exist("Event_IDs"))
    g.createDataSet<int>("Event_IDs", dataspace, props);
  if (!g.hasAttribute("run") && !g.hasAttribute("lumisec")) {
    auto r = g.createAttribute<int>("run", DataSpace::From(iEventID.run));
    auto l = g.createAttribute<int>("lumisec", DataSpace::From(iEventID.lumi));
    r.write(iEventID.run);
    l.write(iEventID.lumi);
  } 
  
  int i = 0; //for data product indices
  for(auto const& s: iSerializers) {
    std::string dp_name{s.name()};
    dataProductIndices_.push_back({dp_name, i});
    ++i;
    std::string dp_sz = dp_name+"_sz";
    if(!g.exist(dp_name) && !g.exist(dp_sz)) {
      auto d = g.createDataSet<char>(dp_name, dataspace, props);
      std::string classname(s.className());
      auto a = d.createAttribute<std::string>("classname", DataSpace::From(classname));
      a.write(classname);
      g.createDataSet<size_t>(dp_sz, dataspace, props); 
    } 
  }
}


