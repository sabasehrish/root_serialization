#include "HDFEventOutputer.h"
#include "OutputerFactory.h"
#include "ConfigurationParameters.h"
#include "UnrolledSerializerWrapper.h"
#include "SerializerWrapper.h"
#include "summarize_serializers.h"
#include "lz4.h"
#include <memory>
#include <iostream>
#include <cstring>
#include <cmath>
#include <set>

using namespace cce::tf;
using namespace cce::tf::pds;
using product_t = std::vector<char>; 

namespace {
  template <typename T> 
  void 
  write_ds(hid_t gid, 
           std::string const& dsname, 
           std::vector<T> const& data) {
    constexpr hsize_t ndims = 1;
    auto dset = hdf5::Dataset::open(gid, dsname.c_str()); 
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

HDFEventOutputer::HDFEventOutputer(std::string const& iFileName, unsigned int iNLanes, int iBatchSize, pds::Serialization iSerialization) : 
  file_(hdf5::File::create(iFileName.c_str())),
  maxBatchSize_{iBatchSize},
  serializers_{std::size_t(iNLanes)},
  serialization_{iSerialization},
  serialTime_{std::chrono::microseconds::zero()},
  parallelTime_{0}
  {}

HDFEventOutputer::~HDFEventOutputer() { }

void HDFEventOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& s = serializers_[iLaneIndex];
  switch(serialization_) {
  case pds::Serialization::kRoot:
    {   s = SerializeStrategy::make<SerializeProxy<SerializerWrapper>>(); break; }
  case pds::Serialization::kRootUnrolled:
    {   s = SerializeStrategy::make<SerializeProxy<UnrolledSerializerWrapper>>(); break; }
  }
  s.reserve(iDPs.size());
  offsetsAndBlob_.first.resize(iDPs.size()+1, 0);
  for(auto const& dp: iDPs) {
    s.emplace_back(dp.name(), dp.classType());
  }
  std::cout << "Setting up for Lanes\n"; 
  if(iLaneIndex == 0) {writeFileHeader(s); }
}

void HDFEventOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  auto& laneSerializers = serializers_[iLaneIndex];
  auto group = iCallback.group();
  laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
}

void HDFEventOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto [offsets, buffer] = writeDataProductsToOutputBuffer(serializers_[iLaneIndex]);
  queue_.push(*iCallback.group(), [this, iEventID, iLaneIndex, callback=std::move(iCallback), buffer = std::move(buffer), offsets = std::move(offsets)]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      const_cast<HDFEventOutputer*>(this)->output(iEventID, serializers_[iLaneIndex], std::move(buffer), std::move(offsets));
        serialTime_ += std::chrono::duration_cast<decltype(serialTime_)>(std::chrono::high_resolution_clock::now() - start);
      callback.doneWaiting();
    });
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    parallelTime_ += time.count();
}

void HDFEventOutputer::printSummary() const  {
  std::cout <<"HDFEventOutputer\n  total serial time at end event: "<<serialTime_.count()<<"us\n"
    "  total parallel time at end event: "<<parallelTime_.load()<<"us\n";
  summarize_serializers(serializers_);
}



void 
HDFEventOutputer::output(EventIdentifier const& iEventID, 
                         SerializeStrategy const& iSerializers,
                         std::vector<char> iBuffer,
                         std::vector<uint32_t> iOffsets ) {
  hdf5::Group gid = hdf5::Group::open(file_, "Lumi");   
  if (firstEvent_) {
    firstEvent_ = false;
     auto r = hdf5::Attribute::open(gid, "run");
     r.write(iEventID.run);  
     auto sr = hdf5::Attribute::open(gid, "lumisec");
     sr.write(iEventID.lumi);  
  }
  eventID_ = iEventID;
  std::vector<unsigned long long> ids;
  ids.push_back(iEventID.event);
  write_ds<unsigned long long>(gid, "Event_IDs", ids);
  write_ds<char>(gid, "products", iBuffer);
  std::vector<uint32_t> offset;
  offset.resize(iOffsets.size()); 
 // std::copy( iOffsets.begin(), iOffsets.end(), offset.begin() );
  write_ds<uint32_t>(gid, "offsets", iOffsets); 
}

void 
HDFEventOutputer::writeFileHeader(SerializeStrategy const& iSerializers) {
  constexpr hsize_t ndims = 1;
  constexpr hsize_t     dims[ndims] = {0};
  constexpr hsize_t     chunk_dims[ndims] = {128};
  constexpr hsize_t     max_dims[ndims] = {H5S_UNLIMITED};
  auto space = hdf5::Dataspace::create_simple (ndims, dims, max_dims); 
  auto prop   = hdf5::Property::create();
  prop.set_chunk(ndims, chunk_dims);
  hdf5::Group g = hdf5::Group::create(file_, "Lumi");
  hdf5::Dataset::create<int>(g, "Event_IDs", space, prop);
  hdf5::Dataset::create<char>(g, "products", space, prop);
  hdf5::Dataset::create<int>(g, "offsets", space, prop);

  const auto scalar_space  = hdf5::Dataspace::create_scalar();
  hdf5::Attribute::create<int>(g, "run", scalar_space);
  hdf5::Attribute::create<int>(g, "lumisec", scalar_space);
  
  for(auto const& s: iSerializers) {
    std::string const type(s.className());
    std::string const name(s.name());
    constexpr hsize_t     str_dims[ndims] = {10};
    auto const attr_type = H5Tcopy (H5T_C_S1);
    H5Tset_size(attr_type, H5T_VARIABLE);
    auto const attr_space  = H5Screate(H5S_SCALAR);
    hdf5::Attribute prod_name = hdf5::Attribute::create<std::string>(g, name.c_str(), attr_space); 
    prod_name.write<std::string>(type);
  }
}

std::pair<std::vector<uint32_t>, std::vector<char>> HDFEventOutputer::writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const{
  //Calculate buffer size needed
  uint32_t bufferSize = 0;
  std::vector<uint32_t> offsets;
  offsets.reserve(iSerializers.size()+1);
  for(auto const& s: iSerializers) {
    auto const blobSize = s.blob().size();
    offsets.push_back(bufferSize);
    bufferSize += blobSize;
  }
  offsets.push_back(bufferSize);

  //initialize with 0
  std::vector<char> buffer(bufferSize, 0);
  
  {
    uint32_t index = 0;
    for(auto const& s: iSerializers) {
      auto offset = offsets[index++];
      std::copy(s.blob().begin(), s.blob().end(), buffer.begin()+offset );
    }
    assert(buffer.size() == offset[index]);
  }

  return {offsets, buffer};
}
namespace {
  class HDFEventMaker : public OutputerMakerBase {
  public:
    HDFEventMaker(): OutputerMakerBase("HDFEventOutputer") {}
    
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {
      auto fileName = params.get<std::string>("fileName");
      if(not fileName) {
        std::cout<<" no file name given for HDFEventOutputer\n";
        return {};
      }

      auto batchSize = params.get<int>("batchSize", 1);
      auto serializationName = params.get<std::string>("serializationAlgorithm", "ROOT");
      auto serialization = pds::toSerialization(serializationName);
      if(not serialization) {
        std::cout <<"unknown serialization "<<serializationName<<std::endl;
        return {};
      }

      return std::make_unique<HDFEventOutputer>(*fileName, iNLanes, batchSize, *serialization);
    }
  };

  HDFEventMaker s_maker;
}
