#include "HDFOutputer.h"
#include "summarize_serializers.h"
#include "lz4.h"
#include <iostream>
#include <cstring>
#include <set>

using namespace cce::tf;
using product_t = std::vector<char>; 

HDFOutputer::HDFOutputer(std::string const& iFileName, unsigned int iNLanes) : 
  file_(File::create(iFileName.c_str())),
  serializers_{std::size_t(iNLanes)},
  serialTime_{std::chrono::microseconds::zero()},
  parallelTime_{0}
  {}

HDFOutputer::~HDFOutputer() { }

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
    sizes.push_back(offsets_[prod_index]+=input[j].size());
    products.insert(end(products), std::make_move_iterator(begin(input[j])), std::make_move_iterator(end(input[j])));
  }
  return {products, sizes};
}

template <typename T> 
void 
write_ds(hid_t gid, 
         std::string name, 
         std::vector<T> data) {
  const hsize_t ndims = 1;
  auto dset = Dataset::open(gid, name.c_str()); 
  auto old_fspace = Dataspace::get_space(dset);
  hsize_t max_dims[ndims] = {H5S_UNLIMITED};
  hsize_t old_dims[ndims]; //our datasets are 1D
  H5Sget_simple_extent_dims(old_fspace, old_dims, max_dims);
  //now old_dims[0] has existing length
  //we need to extend by the size of data
  hsize_t new_dims[ndims];
  new_dims[0] = old_dims[0] + data.size();
  hsize_t slab_size[ndims];
  slab_size[0] = data.size();
  dset.set_extent(new_dims);
  auto new_fspace = Dataspace::get_space (dset);
  new_fspace.select_hyperslab(old_dims, slab_size);
  auto mem_space = Dataspace::create_simple(ndims, slab_size, max_dims);
  dset.write<T>(mem_space, new_fspace, data); //H5Dwrite
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
  if (batch_ == 2) { //max_batch_size){
    Group gid = Group::open(file_, "Lumi");   
    write_ds<int>(gid, "Event_IDs", events_);
    auto const dpi_size = dataProductIndices_.size();
    for(auto & [name, index]: dataProductIndices_) {
      auto [prods, sizes] = get_prods_and_sizes(products_, index, dpi_size);
      write_ds<char>(gid, name, prods);
      auto s = name+"_sz";
      write_ds<size_t>(gid, s, sizes);
    }
    batch_ = 0;
    products_.clear();
    events_.clear();
  }
}

void 
HDFOutputer::writeFileHeader(EventIdentifier const& iEventID, 
                             std::vector<SerializerWrapper> const& iSerializers) {
  const hsize_t ndims = 1;
  constexpr hsize_t     dims[ndims] = {0};
  constexpr hsize_t     chunk_dims[ndims] = {128};
  constexpr hsize_t     max_dims[ndims] = {H5S_UNLIMITED};
  auto space = Dataspace::create_simple (ndims, dims, max_dims); 
  auto prop   = Property::create();
  prop.set_chunk(ndims, chunk_dims);
  Group g = Group::create(file_, "Lumi");
  Dataset dset = Dataset::create<int>(g, "Event_IDs", space, prop);

  const auto scalar_space  = Dataspace::create_scalar();
  Attribute r = Attribute::create<int>(g, "run", scalar_space);
  Attribute l = Attribute::create<int>(g, "lumisec", scalar_space);
  r.write<int>(iEventID.run);
  l.write<int>(iEventID.lumi);
  int dp_index = 0; //for data product indices
  
  for(auto const& s: iSerializers) {
    std::string dp_name{s.name()};
    dataProductIndices_.push_back({dp_name, dp_index});
    ++dp_index;
    std::string dp_sz = dp_name+"_sz";
    Dataset d = Dataset::create<char>(g, dp_name.c_str(), space, prop);
    std::string classname(s.className());
    Attribute a = Attribute::create<std::string>(d, "classname", scalar_space);
    a.write<std::string>(classname);
    Dataset::create<size_t>(g, dp_sz.c_str(), space, prop);
  }
}



