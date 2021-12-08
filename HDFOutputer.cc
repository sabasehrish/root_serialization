#include "HDFOutputer.h"
#include "summarize_serializers.h"
#include "lz4.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <set>
#include "H5Timing.h"
#include "multidataset_plugin.h"
#include <hdf5_hl.h>

int max_batch_size = 2;
int total_n_events = -1;

using namespace cce::tf;
using product_t = std::vector<char>; 

namespace {
  template <typename T> 
  void 
  write_ds(hid_t gid, 
           std::string name, 
           std::vector<T> const& data) {
    constexpr hsize_t ndims = 1;
    auto dset = hdf5::Dataset::open(gid, name.c_str()); 
    auto old_fspace = hdf5::Dataspace::get_space(dset);
    hsize_t max_dims[ndims]; //= {H5S_UNLIMITED};
    hsize_t old_dims[ndims]; //our datasets are 1D
    H5Sget_simple_extent_dims(old_fspace, old_dims, max_dims);
    //now old_dims[0] has existing length
    //we need to extend by the size of data
    hsize_t new_dims[ndims];
    new_dims[0] = old_dims[0] + data.size();
    hsize_t slab_size[ndims];
    slab_size[0] = data.size();
    dset.set_extent(new_dims);
    auto new_fspace = hdf5::Dataspace::get_space (dset);
    new_fspace.select_hyperslab(old_dims, slab_size);
    auto mem_space = hdf5::Dataspace::create_simple(ndims, slab_size, max_dims);
    dset.write<T>(mem_space, new_fspace, data); //H5Dwrite
  }
}

int append_dataset(hid_t gid, const char *name, char* data, size_t data_size, hid_t mtype) {
  hid_t did = H5Dopen2(gid, name, H5P_DEFAULT);
  H5DOappend( did, H5P_DEFAULT, 0, data_size, mtype, data);
  H5Dclose(did);
  return 0;
}

int write_multidatasets(hid_t gid, const char *name, char* data, size_t data_size, hid_t mtype) {
  const hsize_t ndims = 1;
  hid_t did, dsid, msid;
  hsize_t max_dims[ndims]; //= {H5S_UNLIMITED};
  hsize_t old_dims[ndims]; //our datasets are 1D
  hsize_t new_dims[ndims];
  hsize_t slab_size[ndims];

  did = H5Dopen2(gid, name, H5P_DEFAULT);
  dsid = H5Dget_space(did);
  H5Sget_simple_extent_dims(dsid, old_dims, max_dims);
  new_dims[0] = old_dims[0] + data_size;
  slab_size[0] = data_size;
  H5Dset_extent(did, new_dims);
  H5Sclose(dsid);

  dsid = H5Dget_space(did);
  H5Sselect_hyperslab(dsid, H5S_SELECT_SET, old_dims, NULL, slab_size, NULL);
  msid = H5Screate_simple(ndims, slab_size, max_dims);

  register_multidataset(name, data, did, dsid, msid, mtype, 1);

  return 0;
}

HDFOutputer::HDFOutputer(std::string const& iFileName, unsigned int iNLanes) : 
  file_(hdf5::File::create(iFileName.c_str())),
  serializers_{std::size_t(iNLanes)},
  serialTime_{std::chrono::microseconds::zero()},
  parallelTime_{0}
  {}

HDFOutputer::~HDFOutputer() { }

void HDFOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& s = serializers_[iLaneIndex];
  s.reserve(iDPs.size());
  dataProductIndices_.reserve(iDPs.size());
  for(auto const& dp: iDPs) {
    s.emplace_back(dp.name(), dp.classType());
  }
   products_.reserve(iDPs.size() * max_batch_size);
   events_.reserve(max_batch_size);
   offsets_.reserve(max_batch_size);
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
  sizes.reserve(input.size()); 
  // or may use (std::ceil(double(input.size()-prod_index)/stride)); 
  for(int j = prod_index; j< input.size(); j+=stride) {
    sizes.push_back(offsets_[prod_index]+=input[j].size());
    products.insert(end(products), std::make_move_iterator(begin(input[j])), std::make_move_iterator(end(input[j])));
  }
  return {products, sizes};
}



void 
HDFOutputer::output(EventIdentifier const& iEventID, 
                    std::vector<SerializerWrapper> const& iSerializers) {
#ifdef H5_TIMING_ENABLE
  size_t total_data_size = 0;
#endif
  if(firstTime_) {
    writeFileHeader(iEventID, iSerializers);
    firstTime_ = false;
  }
  // accumulate events before writing, go through all the data products in the curret event
  for(auto& s: iSerializers) 
     products_.push_back(s.blob());
  events_.push_back(iEventID.event);

  ++batch_;
  if (total_n_events > 0) {
    total_n_events--;
  }

  if (batch_ == max_batch_size || total_n_events == 0) {
    hdf5::Group gid = hdf5::Group::open(file_, "Lumi");   
    write_ds<int>(gid, "Event_IDs", events_);
    auto const dpi_size = dataProductIndices_.size();
    for(auto & [name, index]: dataProductIndices_) {
      auto [prods, sizes] = get_prods_and_sizes(products_, index, dpi_size);
#ifdef H5_TIMING_ENABLE
      register_dataset_timer_start(name.c_str());
#endif
      //write_ds<char>(gid, name, prods);
      write_multidatasets(gid, name.c_str(), (char*) &(prods[0]), prods.size(), H5T_NATIVE_CHAR);
      //append_dataset(gid, name.c_str(), (char*) &(prods[0]), prods.size(), H5T_NATIVE_CHAR);
#ifdef H5_TIMING_ENABLE
      register_dataset_timer_end((size_t)prods.size());
#endif
      auto s = name+"_sz";
#ifdef H5_TIMING_ENABLE
      register_dataset_sz_timer_start(s.c_str());
#endif
      //write_ds<size_t>(gid, s, sizes);
      write_multidatasets(gid, s.c_str(), (char*) &(sizes[0]), sizes.size(), H5T_NATIVE_ULLONG);
      //append_dataset(gid, s.c_str(), (char*) &(sizes[0]), sizes.size(), H5T_NATIVE_ULLONG);
#ifdef H5_TIMING_ENABLE
      register_dataset_sz_timer_end((size_t)sizes.size() * sizeof(size_t));
#endif
#ifdef H5_TIMING_ENABLE
      total_data_size += (size_t)prods.size() + (size_t)sizes.size() * sizeof(size_t);
#endif
    }
    batch_ = 0;
    products_.clear();
    events_.clear();
  }
  check_write_status();
  if (total_n_events == 0) {
#ifdef H5_TIMING_ENABLE
    register_dataset_timer_start("flush_all");
#endif
    flush_multidatasets();
    dataset_recycle_all();
    dataspace_recycle_all();
    memspace_recycle_all();
#ifdef H5_TIMING_ENABLE
    register_dataset_timer_end(total_data_size);
#endif
  }

}

void 
HDFOutputer::writeFileHeader(EventIdentifier const& iEventID, 
                             std::vector<SerializerWrapper> const& iSerializers) {
  constexpr hsize_t ndims = 1;
  constexpr hsize_t     dims[ndims] = {0};
  constexpr hsize_t     chunk_dims[ndims] = {128};
  constexpr hsize_t     max_dims[ndims] = {H5S_UNLIMITED};
  auto space = hdf5::Dataspace::create_simple (ndims, dims, max_dims); 
  auto prop   = hdf5::Property::create();
  prop.set_chunk(ndims, chunk_dims);
  hdf5::Group g = hdf5::Group::create(file_, "Lumi");
  hdf5::Dataset dset = hdf5::Dataset::create<int>(g, "Event_IDs", space, prop);

  const auto scalar_space  = hdf5::Dataspace::create_scalar();
  hdf5::Attribute r = hdf5::Attribute::create<int>(g, "run", scalar_space);
  hdf5::Attribute l = hdf5::Attribute::create<int>(g, "lumisec", scalar_space);
  r.write<int>(iEventID.run);
  l.write<int>(iEventID.lumi);
  int dp_index = 0; //for data product indices
  
  for(auto const& s: iSerializers) {
    std::string dp_name{s.name()};
    dataProductIndices_.emplace_back(dp_name, dp_index);
    ++dp_index;
    std::string dp_sz = dp_name+"_sz";
    hdf5::Dataset d = hdf5::Dataset::create<char>(g, dp_name.c_str(), space, prop);
    std::string classname(s.className());
    hdf5::Attribute a = hdf5::Attribute::create<std::string>(d, "classname", scalar_space);
    a.write<std::string>(classname);
    hdf5::Dataset::create<size_t>(g, dp_sz.c_str(), space, prop);
  }
}



