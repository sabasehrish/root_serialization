#include "HDFOutputer.h"
#include "OutputerFactory.h"
#include "ConfigurationParameters.h"
#include "summarize_serializers.h"
#include "lz4.h"
#include <memory>
#include <iostream>
#include <cstring>
#include <cmath>
#include <set>
#include <hdf5_hl.h>
#include "multidataset_plugin.h"
#include "H5Timing.h"

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
  register_multidataset_request_append(name, gid, data, data_size, mtype);
  return 0;
}

HDFOutputer::HDFOutputer(std::string const& iFileName, unsigned int iNLanes, int iBatchSize, int iChunkSize) : 
  file_(hdf5::File::create(iFileName.c_str())),
  chunkSize_{iChunkSize},
  maxBatchSize_{iBatchSize},
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
  products_.reserve(iDPs.size() * maxBatchSize_);
  events_.reserve(maxBatchSize_);
  offsets_.reserve(maxBatchSize_);

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

  auto start = std::chrono::high_resolution_clock::now();
  if (batch_ != 0) {
    //flush the remaining data to the file
    const_cast<HDFOutputer*>(this)->writeBatch();
  }

  finalize_multidataset();
  auto writeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  
  std::cout << "  end of job file write time: "<<writeTime.count()<<"us\n";

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
  if(firstTime_) {
    writeFileHeader(iEventID, iSerializers);
    firstTime_ = false;
  }
  // accumulate events before writing, go through all the data products in the curret event
  for(auto& s: iSerializers) 
     products_.push_back(s.blob());
  events_.push_back(iEventID.event);

  ++batch_;
  if (batch_ == maxBatchSize_) {
    writeBatch();
    batch_ = 0;
    products_.clear();
    events_.clear();
  }
}

void
HDFOutputer::writeBatch() {
  init_multidataset();

  int method = get_hdf5_method();
#ifdef H5_TIMING_ENABLE
  size_t total_data_size = 0;
#endif
  hdf5::Group gid = hdf5::Group::open(file_, "Lumi");   
  write_ds<int>(gid, "Event_IDs", events_);
  auto const dpi_size = dataProductIndices_.size();
  for(auto & [name, index]: dataProductIndices_) {
      auto [prods, sizes] = get_prods_and_sizes(products_, index, dpi_size);
#ifdef H5_TIMING_ENABLE
      register_dataset_timer_start(name.c_str());
#endif
      if ( method == 1 ) {
        write_ds<char>(gid, name, prods);
      } else if (method == 0 ) {
        write_multidatasets(gid, name.c_str(), (char*) &(prods[0]), prods.size(), H5T_NATIVE_CHAR);
      } else {
        append_dataset(gid, name.c_str(), (char*) &(prods[0]), prods.size(), H5T_NATIVE_CHAR);
      }
#ifdef H5_TIMING_ENABLE
      register_dataset_timer_end((size_t)prods.size());
#endif
      auto s = name+"_sz";
#ifdef H5_TIMING_ENABLE
      register_dataset_sz_timer_start(s.c_str());
#endif
      if (method == 1) {
        write_ds<size_t>(gid, s, sizes);
      } else if ( method == 0 ) {
        write_multidatasets(gid, s.c_str(), (char*) &(sizes[0]), sizes.size(), H5T_NATIVE_ULLONG);
      } else {
        append_dataset(gid, s.c_str(), (char*) &(sizes[0]), sizes.size(), H5T_NATIVE_ULLONG);
      }
#ifdef H5_TIMING_ENABLE
      register_dataset_sz_timer_end((size_t)sizes.size() * sizeof(size_t));
#endif
#ifdef H5_TIMING_ENABLE
      total_data_size += (size_t)prods.size() + (size_t)sizes.size() * sizeof(size_t);
#endif
  }

#ifdef H5_TIMING_ENABLE
  register_dataset_timer_start("flush_all");
#endif
  flush_multidatasets();
#ifdef H5_TIMING_ENABLE
  register_dataset_timer_end(total_data_size);
#endif

}

void 
HDFOutputer::writeFileHeader(EventIdentifier const& iEventID, 
                             std::vector<SerializerWrapper> const& iSerializers) {
  constexpr hsize_t ndims = 1;
  constexpr hsize_t     dims[ndims] = {0};
  const hsize_t     chunk_dims[ndims] = {static_cast<hsize_t>(chunkSize_)};
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

namespace {
  class HDFMaker : public OutputerMakerBase {
  public:
    HDFMaker(): OutputerMakerBase("HDFOutputer") {}
    
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {
      auto fileName = params.get<std::string>("fileName");
      if(not fileName) {
        std::cout<<" no file name given for HDFOutputer\n";
        return {};
      }

      auto batchSize = params.get<int>("batchSize", 1);
      auto chunkSize = params.get<int>("hdfchunkSize", 1048576);

      return std::make_unique<HDFOutputer>(*fileName, iNLanes, batchSize, chunkSize);
    }
  };

  HDFMaker s_maker;
}
