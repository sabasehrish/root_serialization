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
  offsets_.reserve(10000);
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
    std::cout << prod_index << ", " << offsets_[prod_index] << "\n";
    sizes.push_back(offsets_[prod_index]+=input[j].size());
    //sizes.push_back(input[j].size());
    products.insert(end(products), std::make_move_iterator(begin(input[j])), std::make_move_iterator(end(input[j])));
  }
  return {products, sizes};
}

template <typename T> 
void write_ds(hid_t gid, std::string name, std::vector<T> data) {
  hid_t dset = H5Dopen2(gid, name.c_str(), H5P_DEFAULT); 
  hid_t dspace = H5Dget_space(dset);
  hsize_t     maxdims[1] = {H5S_UNLIMITED};
  hsize_t old_dims[1]; //our datasets are 1D
  H5Sget_simple_extent_dims(dspace, old_dims, maxdims);
  //now old_dims[0] has existing length
  //we need to extend by the size of data
  hsize_t new_dims[1];
  new_dims[0] = old_dims[0] + data.size();
  hsize_t stride[1];
  stride[0] = data.size();
  hid_t status = H5Dextend (dset, new_dims);
  auto filespace = H5Dget_space (dset);
  status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, old_dims, NULL, stride, NULL); 
  dspace = H5Screate_simple(1, stride, maxdims);
  //std::cout << "Old_dims: " << old_dims[0] << ", New dims" << new_dims[0] << std::endl;
 // std::cout << "event 0 " << data[0] << ", event 1: " << data[1] << std::endl;
  
  if (typeid(data[0]) == typeid(int)) {
    status = H5Dwrite(dset, H5T_STD_I32LE, dspace, filespace, H5P_DEFAULT, &data[0]);  } else if (typeid(data[0]) == typeid(char)) {
    status = H5Dwrite(dset, H5T_STD_I8LE, dspace, filespace, H5P_DEFAULT, &data[0]);
  } else if (typeid(data[0]) == typeid(size_t)) {
    status = H5Dwrite(dset, H5T_STD_U64LE, dspace, filespace, H5P_DEFAULT, &data[0]);
  } else {
    std::cerr << "We currently dont support this type\n";
  }
H5Dclose(dset);
H5Sclose(dspace);
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
    hid_t gid = H5Gopen2(file1_, "Lumi", H5P_DEFAULT);   
    write_ds<int>(gid, "Event_IDs", events_);
    auto const dpi_size = dataProductIndices_.size();
    for(auto & [name, index]: dataProductIndices_) {
      auto [prods, sizes] = get_prods_and_sizes(products_, index, dpi_size);
      write_ds<char>(gid,name, prods);
      auto s = name+"_sz";
      write_ds<size_t>(gid, s, sizes);
    }
    batch_ = 0;
    products_.clear();
    events_.clear();
    H5Gclose(gid);
  }
}

void HDFOutputer::writeFileHeader(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) {
  hsize_t     dims[1] = {0};
  hsize_t     chunk_dims[1] = {128};
  hsize_t     maxdims[1] = {H5S_UNLIMITED};
  hid_t space = H5Screate_simple (1, dims, maxdims); 
  auto prop   = H5Pcreate(H5P_DATASET_CREATE);
  auto status = H5Pset_chunk(prop, 1, chunk_dims);
  hid_t g = H5Gcreate2(file1_, "Lumi", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t dset = H5Dcreate2(g, "Event_IDs", H5T_STD_I32LE, space, H5P_DEFAULT,
                 prop, H5P_DEFAULT);

  auto aid3  = H5Screate(H5S_SCALAR);
  
  hid_t r = H5Acreate2(g, "run", H5T_STD_I32LE, aid3, H5P_DEFAULT, H5P_DEFAULT);
  hid_t l = H5Acreate2(g, "lumisec", H5T_STD_I32LE, aid3, H5P_DEFAULT, H5P_DEFAULT);
  H5Awrite(r, H5T_STD_I32LE, &iEventID.run);
  H5Awrite(l, H5T_STD_I32LE, &iEventID.lumi);
  H5Aclose(r);
  H5Aclose(l);
   
  int i = 0; //for data product indices
  
  for(auto const& s: iSerializers) {
    std::string dp_name{s.name()};
    dataProductIndices_.push_back({dp_name, i});
    ++i;
    std::string dp_sz = dp_name+"_sz";
    hid_t d = H5Dcreate2(g, dp_name.c_str(), H5T_NATIVE_CHAR, space, H5P_DEFAULT,prop, H5P_DEFAULT);;
    std::string classname(s.className());
    auto atype = H5Tcopy(H5T_C_S1); //for creating string attributes
    H5Tset_size(atype, H5T_VARIABLE);
    H5Tset_cset(atype, H5T_CSET_UTF8);
    H5Tset_strpad(atype,H5T_STR_NULLTERM);
    hid_t a = H5Acreate2(d, "classname", atype, aid3, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(a, atype, &classname);
    auto d1 = H5Dcreate2(g, dp_sz.c_str(), H5T_STD_U64LE, space, H5P_DEFAULT, prop, H5P_DEFAULT);
    H5Dclose(d);
    H5Dclose(d1);
    H5Aclose(a);
  }
  H5Gclose(g);
  status = H5Dclose (dset);
  status = H5Sclose (space);
}



