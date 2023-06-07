#include "PHDFBatchEventsOutputer.h"
#include "OutputerFactory.h"
#include "ConfigurationParameters.h"
#include "UnrolledSerializerWrapper.h"
#include "SerializerWrapper.h"
#include "summarize_serializers.h"
#include "FunctorTask.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <set>

#include "mpi.h"

using namespace cce::tf;
using namespace cce::tf::pds;

namespace {
  constexpr const char* const PRODUCTS_DSNAME="Products";
  constexpr const char* const EVENTS_DSNAME="EventIDs";
  constexpr const char* const OFFSETS_DSNAME="Offsets";
  constexpr const char* const GNAME="Lumi";
  constexpr const char* const RUN_ANAME="run";
  constexpr const char* const LUMISEC_ANAME="lumisec";
  constexpr const char* const COMPRESSION_ANAME="Compression";
  constexpr const char* const COMPRESSION_LEVEL_ANAME="CompressionLevel";
  constexpr const char* const COMPRESSION_CHOICE_ANAME="CompressionChoice";
  
  template <typename T> 
  std::tuple<std::chrono::microseconds, std::chrono::microseconds, std::chrono::microseconds, std::chrono::microseconds, std::chrono::microseconds, std::chrono::microseconds> 
  write_ds(hid_t gid, 
           std::string const& dsname, 
           std::vector<T> const& data,
           MPI_Comm comm) {
    constexpr hsize_t ndims = 1;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto dset = hdf5::Dataset::open(gid, dsname.c_str()); 
    std::chrono::microseconds dsopentime = std::chrono::duration_cast<decltype(dsopentime)>(std::chrono::high_resolution_clock::now() - start);
    start = std::chrono::high_resolution_clock::now();
    auto old_fspace = hdf5::Dataspace::get_space(dset);
    std::chrono::microseconds getspacetime = std::chrono::duration_cast<decltype(getspacetime)>(std::chrono::high_resolution_clock::now() - start);
    hsize_t max_dims[ndims]; 
    hsize_t old_dims[ndims]; //our datasets are 1D
    H5Sget_simple_extent_dims(old_fspace, old_dims, max_dims);
    hsize_t data_size[ndims];
    data_size[0] = data.size();
    //Call to MPI_Scan followed by AllReduce
    //AllReduce will let every rank know the max dim 
    int partial_sum_size[ndims] = {0};
    
    start = std::chrono::high_resolution_clock::now();
    MPI_Scan(data_size, partial_sum_size, ndims, MPI_INT, MPI_SUM, comm);
    std::chrono::microseconds scantime = std::chrono::duration_cast<decltype(scantime)>(std::chrono::high_resolution_clock::now() - start);
    
    int max_size[ndims] = {0};
    start = std::chrono::high_resolution_clock::now();
    max_size[0] = 0;//partial_sum_size[0];
    //MPI_Allreduce(partial_sum_size, max_size, ndims, MPI_INT, MPI_MAX, comm);
    //last rank's partial_sum_size will be the max_size by definition
    //so we can replace the MPI_Allreduce with an MPI_Bcast by the last rank
    // MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
    int nranks;
    MPI_Comm_size (MPI_COMM_WORLD, &nranks);
    int last_rank = nranks-1;
    MPI_Bcast(max_size, ndims, MPI_INT, last_rank, comm);
    std::chrono::microseconds reducetime = std::chrono::duration_cast<decltype(reducetime)>(std::chrono::high_resolution_clock::now() - start);
    hsize_t new_dims[ndims];
    new_dims[0] = old_dims[0] + max_size[0];
    start = std::chrono::high_resolution_clock::now();
    dset.set_extent(new_dims);
    std::chrono::microseconds setextenttime = std::chrono::duration_cast<decltype(setextenttime)>(std::chrono::high_resolution_clock::now() - start);
    hsize_t slab_size[ndims];
    slab_size[0] = data.size();
    hsize_t slab_offset[ndims] = {old_dims[0] + partial_sum_size[0] - slab_size[0]};
    auto new_fspace = hdf5::Dataspace::get_space(dset);
    new_fspace.select_hyperslab(slab_offset, slab_size);
    auto mem_space = hdf5::Dataspace::create_simple(ndims, slab_size, max_dims);
    start = std::chrono::high_resolution_clock::now();
    dset.parallel_write<T>(mem_space, new_fspace, data); //H5Dwrite
    std::chrono::microseconds writetime = std::chrono::duration_cast<decltype(writetime)>(std::chrono::high_resolution_clock::now() - start);
    return {dsopentime, getspacetime, setextenttime, scantime, reducetime, writetime};
  }

}

PHDFBatchEventsOutputer::PHDFBatchEventsOutputer(std::string const& iFileName, unsigned int iNLanes, int iChunkSize, pds::Compression iCompression, int iCompressionLevel, CompressionChoice iChoice, pds::Serialization iSerialization, uint32_t iBatchSize, int nEvents) : 
  file_(hdf5::File::parallel_create(iFileName.c_str())),
  group_(hdf5::Group::create(file_, GNAME)),
  chunkSize_{iChunkSize},
  serializers_{std::size_t(iNLanes)},
  eventBatches_{iNLanes},
  waitingEventsInBatch_(iNLanes),
  presentEventEntry_(0),
  batchSize_(iBatchSize),
  nEvents_(nEvents),
  localEventcounter_(0),
  localEventswritten_(0),
  compression_{iCompression},
  compressionLevel_{iCompressionLevel},
  compressionChoice_{iChoice},
  serialization_{iSerialization},
  serialTime_{std::chrono::microseconds::zero()},
  parallelTime_{0},
  h5dsopenTime_{std::chrono::microseconds::zero()},
  h5getspaceTime_{std::chrono::microseconds::zero()},
  h5setextentTime_{std::chrono::microseconds::zero()},
  mpiscanTime_{std::chrono::microseconds::zero()},
  mpireduceTime_{std::chrono::microseconds::zero()},
  h5dswriteTime_{std::chrono::microseconds::zero()},
  fileheaderTime_{std::chrono::microseconds::zero()}
  {
    for(auto& v: waitingEventsInBatch_) {
      v.store(0);
    }
    for(auto& v:eventBatches_) {
      v.store(nullptr);
    }
    
  }


void PHDFBatchEventsOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& s = serializers_[iLaneIndex];
  switch(serialization_) {
  case pds::Serialization::kRoot:
    {   s = SerializeStrategy::make<SerializeProxy<SerializerWrapper>>(); break; }
  case pds::Serialization::kRootUnrolled:
    {   s = SerializeStrategy::make<SerializeProxy<UnrolledSerializerWrapper>>(); break; }
  }
  s.reserve(iDPs.size());
  for(auto const& dp: iDPs) {
    s.emplace_back(dp.name(), dp.classType());
  }
  if(iLaneIndex == 0) { //and my rank == 0 
    auto start = std::chrono::high_resolution_clock::now();
    writeFileHeader(s);
    fileheaderTime_ = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);
  }
}

void PHDFBatchEventsOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  auto& laneSerializers = serializers_[iLaneIndex];
  auto group = iCallback.group();
  laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
}

void PHDFBatchEventsOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto [offsets, buffer] = writeDataProductsToOutputBuffer(serializers_[iLaneIndex]);
  
  auto eventIndex = presentEventEntry_++;
  auto batchIndex = (eventIndex/batchSize_) % eventBatches_.size();
  
  auto& batchContainerPtr = eventBatches_[batchIndex];
  auto batchContainer = batchContainerPtr.load();
  if( nullptr == batchContainer ) {
    auto newContainer = std::make_unique<std::vector<EventInfo>>(batchSize_);
    if(batchContainerPtr.compare_exchange_strong(batchContainer, newContainer.get())) {
      newContainer.release();
      batchContainer = batchContainerPtr.load();
    }
  }

  auto indexInBatch = eventIndex % (batchSize_);
  (*batchContainer)[indexInBatch] = std::make_tuple(iEventID, std::move(offsets), std::move(buffer));

  assert(waitingEventsInBatch_[batchIndex].load() < batchSize_);

  auto waitingEvents = ++waitingEventsInBatch_[batchIndex];
  if(waitingEvents == batchSize_ ) {
    const_cast<PHDFBatchEventsOutputer*>(this)->finishBatchAsync(batchIndex, std::move(iCallback));
  }
  if(waitingEvents == batchSize_ ) {
  auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  parallelTime_ += time.count();
  }
}
void PHDFBatchEventsOutputer::printSummary() const  {
  //make sure last batches are out

  auto start = std::chrono::high_resolution_clock::now();
  {
    tbb::task_group group;
    
    {
      TaskHolder th(group, make_functor_task([](){}));
      for( int index=0; index < waitingEventsInBatch_.size();++index) {
        if(0 != waitingEventsInBatch_[index].load()) {
          const_cast<PHDFBatchEventsOutputer*>(this)->finishBatchAsync(index, th);
        }
      }
    }
    
    group.wait();
  }
  int my_rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
  int nranks;
  MPI_Comm_size (MPI_COMM_WORLD, &nranks);
  std::string timingfile = "Timing_"+std::to_string(my_rank)+".txt";
  std::ofstream fout(timingfile, std::ios_base::app);
  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);
  auto writeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  fout << "PHDFBatchEventsOutputer stats for rank " << my_rank << " of " << nranks << " total ranks\n";
  fout << "Total serial time at end event: "<<serialTime_.count()<<"us\n";
  fout << "Total parallel time at end event: "<<parallelTime_.load()<<"us\n";
  fout << "End of job file write time: "<<writeTime.count()<<"us\n";
  fout << "Total time at end event: "<<serialTime_.count()+parallelTime_.load()+writeTime.count()<<"us\n";
  fout << "Compression: "<< std::string(name(compression_)) << "\n";
  fout << "Compression Level: "<< compressionLevel_ << "\n";
  fout << "Compression Choice: "<< static_cast<int>(compressionChoice_) << "\n";
  fout << "Batch Size: "<< batchSize_ << "\n";
  fout << "Chunk Size: "<< chunkSize_ << "\n";
  fout << "Events written: "<< localEventswritten_ << "\n";
  fout << "HDF5 dataset open time: "<< h5dsopenTime_.count() << "\n";
  fout << "HDF5 get space time: "<< h5getspaceTime_.count() << "\n";
  fout << "HDF5 set extent time: "<< h5setextentTime_.count() << "\n";
  fout << "MPI Scan time: "<< mpiscanTime_.count() << "\n";
  fout << "MPI Broadcast time: "<< mpireduceTime_.count() << "\n"; //did not change the variable name, but this does capture bcast time
  fout << "HDF5 datasets write time: "<< h5dswriteTime_.count() << "\n";
  fout << "HDF5 file header write time: "<< fileheaderTime_.count() << "\n";

  summarize_serializers(serializers_);
}

void PHDFBatchEventsOutputer::finishBatchAsync(unsigned int iBatchIndex, TaskHolder iCallback) {

  std::unique_ptr<std::vector<EventInfo>> batch(eventBatches_[iBatchIndex].exchange(nullptr));
  auto eventsInBatch = waitingEventsInBatch_[iBatchIndex].load();
  waitingEventsInBatch_[iBatchIndex] = 0;

  std::vector<EventIdentifier> batchEventIDs;
  batchEventIDs.reserve(batch->size());

  std::vector<uint32_t> batchOffsets;
  batchOffsets.reserve(batch->size() * (serializers_.size()+2));
  //one extra size to but the final blob size. This is either the
  // compressed size or the uncompressed size depending on the compression choice

  std::vector<char> batchBlob;
  int rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  int nranks;
  MPI_Comm_size (MPI_COMM_WORLD, &nranks);
  int index = 0;
  for(auto& [id, offsets, blob]: *batch) {
    if (firstEvent_) {
      localEventcounter_=nEvents_+id.event;
      firstEvent_ = false;
    }
    //std::cout << "Local and nevents: " << localEventcounter_ << ", " << id.event << std::endl;
    if(rank == (nranks-1) && localEventcounter_ <= id.event) { 
      break;
    }
    if(index++ == eventsInBatch) { 
      //batch was smaller than usual. Can happen at end of job
      break;
    }

    ++localEventswritten_;
    batchEventIDs.push_back(id);

    std::copy(offsets.begin(), offsets.end(), std::back_inserter(batchOffsets));

    //record the size of the blob as the final offset. Needed to decompress
    // the event during reading
    batchOffsets.push_back(blob.size());
    std::copy(blob.begin(), blob.end(), std::back_inserter(batchBlob));

    //release memory
    blob = {};
  }

  std::vector<char> bufferToWrite;
  if(compressionChoice_ == CompressionChoice::kBatch or compressionChoice_ == CompressionChoice::kBoth) {
    bufferToWrite  = pds::compressBuffer(0,0, compression_, compressionLevel_, batchBlob);
    batchBlob = std::vector<char>();
  } else {
    bufferToWrite = std::move(batchBlob);
  }

  
  queue_.push(*iCallback.group(), [this, eventIDs=std::move(batchEventIDs), offsets = std::move(batchOffsets), buffer = std::move(bufferToWrite),  callback=std::move(iCallback)]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      const_cast<PHDFBatchEventsOutputer*>(this)->output(std::move(eventIDs), std::move(buffer), std::move(offsets));
        serialTime_ += std::chrono::duration_cast<decltype(serialTime_)>(std::chrono::high_resolution_clock::now() - start);
      callback.doneWaiting();
    });
  
}

void 
PHDFBatchEventsOutputer::output(std::vector<EventIdentifier> iEventIDs, 
                         std::vector<char> iBuffer,
                         std::vector<uint32_t> iOffsets ) {
  if (writefirstEvent_) {
    assert(not iEventIDs.empty());
    writefirstEvent_ = false;
    auto r = hdf5::Attribute::open(group_, RUN_ANAME);
    r.write(iEventIDs[0].run);  
    auto sr = hdf5::Attribute::open(group_, LUMISEC_ANAME);
    sr.write(iEventIDs[0].lumi); 
    auto comp = hdf5::Attribute::open(group_, COMPRESSION_ANAME);
    comp.write(std::string(name(compression_)));
    auto level = hdf5::Attribute::open(group_, COMPRESSION_LEVEL_ANAME);
    level.write(compressionLevel_); 
    auto choice = hdf5::Attribute::open(group_, COMPRESSION_CHOICE_ANAME);
    choice.write(static_cast<int>(compressionChoice_)); 
  }
  std::vector<unsigned long long> ids;
  ids.reserve(iEventIDs.size());
  std::transform(iEventIDs.begin(), iEventIDs.end(), std::back_inserter(ids), [](auto const&id) {return id.event;});
  std::chrono::microseconds a;
  std::chrono::microseconds b;
  std::chrono::microseconds c;
  std::chrono::microseconds d;
  std::chrono::microseconds e;
  std::chrono::microseconds f;
  std::tie(a, b, c, d, e, f) = write_ds<unsigned long long>(group_, EVENTS_DSNAME, ids, MPI_COMM_WORLD);
  h5dsopenTime_+=a;
  h5getspaceTime_+=b;
  h5setextentTime_+=c;
  mpiscanTime_+=d;
  mpireduceTime_+=e;
  h5dswriteTime_+=f;
  std::tie(a, b, c, d, e, f) = write_ds<char>(group_, PRODUCTS_DSNAME, iBuffer, MPI_COMM_WORLD);
  h5dsopenTime_+=a;
  h5getspaceTime_+=b;
  h5setextentTime_+=c;
  mpiscanTime_+=d;
  mpireduceTime_+=e;
  h5dswriteTime_+=f;
  std::tie(a, b, c, d, e, f) = write_ds<uint32_t>(group_, OFFSETS_DSNAME, iOffsets, MPI_COMM_WORLD);
  h5dsopenTime_+=a;
  h5getspaceTime_+=b;
  h5setextentTime_+=c;
  mpiscanTime_+=d;
  mpireduceTime_+=e;
  h5dswriteTime_+=f;
       
}

void 
PHDFBatchEventsOutputer::writeFileHeader(SerializeStrategy const& iSerializers) {
  constexpr hsize_t ndims = 1;
  constexpr hsize_t dims[ndims] = {0};
  hsize_t chunk_dims[ndims] = {static_cast<hsize_t>(chunkSize_)};
  constexpr hsize_t  max_dims[ndims] = {H5S_UNLIMITED};
  auto space = hdf5::Dataspace::create_simple (ndims, dims, max_dims); 
  auto prop   = hdf5::Property::create();
  prop.set_chunk(ndims, chunk_dims);
  auto es = hdf5::Dataset::create<int>(group_, EVENTS_DSNAME, space, prop);
  auto ps = hdf5::Dataset::create<char>(group_, PRODUCTS_DSNAME, space, prop);
  auto os = hdf5::Dataset::create<int>(group_, OFFSETS_DSNAME, space, prop);

  const auto scalar_space  = hdf5::Dataspace::create_scalar();
  auto a1 = hdf5::Attribute::create<int>(group_, RUN_ANAME, scalar_space);
  auto a2 = hdf5::Attribute::create<int>(group_, LUMISEC_ANAME, scalar_space);
  auto a3 = hdf5::Attribute::create<int>(group_, COMPRESSION_LEVEL_ANAME, scalar_space);
  auto a4 = hdf5::Attribute::create<int>(group_, COMPRESSION_CHOICE_ANAME, scalar_space);
  constexpr hsize_t     str_dims[ndims] = {10};
  auto const attr_type = H5Tcopy (H5T_C_S1);
  H5Tset_size(attr_type, H5T_VARIABLE);
  auto const attr_space  = H5Screate(H5S_SCALAR);
  hdf5::Attribute compression = hdf5::Attribute::create<std::string>(group_,COMPRESSION_ANAME, attr_space);
  for(auto const& s: iSerializers) {
    std::string const type(s.className());
    std::string const name(s.name());
    hdf5::Attribute prod_name = hdf5::Attribute::create<std::string>(group_, name.c_str(), attr_space); 
    prod_name.write<std::string>(type);
  }
}

std::pair<std::vector<uint32_t>, std::vector<char>> PHDFBatchEventsOutputer::writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const{
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
    assert(buffer.size() == offsets[index]);
  }

  if(compressionChoice_ == CompressionChoice::kEvents or compressionChoice_ == CompressionChoice::kBoth) {
    auto cBuffer  = pds::compressBuffer(0,0, compression_, compressionLevel_, buffer);

    return {std::move(offsets), std::move(cBuffer)};
  }

  return {std::move(offsets), std::move(buffer)};
}
namespace {
  class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("PHDFBatchEventsOutputer") {}
    
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params, int nEvents) const final {
      auto fileName = params.get<std::string>("fileName");
      if(not fileName) {
        std::cout<<" no file name given for PHDFBatchEventsOutputer\n";
        return {};
      }

      auto chunkSize = params.get<int>("hdfchunkSize", 10485760);
      int compressionLevel = params.get<int>("compressionLevel", 18);
      auto compressionName = params.get<std::string>("compressionAlgorithm", "ZSTD");
      auto compression = pds::toCompression(compressionName);
      if(not compression) {
        std::cout <<"unknown compression "<<compressionName<<std::endl;
        return {};
      }
      auto serializationName = params.get<std::string>("serializationAlgorithm", "ROOT");
      auto serialization = pds::toSerialization(serializationName);
      if(not serialization) {
        std::cout <<"unknown serialization "<<serializationName<<std::endl;
        return {};
      }
      auto compressionChoiceName = params.get<std::string>("compressionChoice", "Events");
      auto compressionChoice = PHDFBatchEventsOutputer::CompressionChoice::kEvents;
      if(compressionChoiceName == "Events") {
      }else if(compressionChoiceName == "Batch") {
        compressionChoice = PHDFBatchEventsOutputer::CompressionChoice::kBatch;
      }else if(compressionChoiceName == "Both") {
        compressionChoice = PHDFBatchEventsOutputer::CompressionChoice::kBoth;
      }else if(compressionChoiceName == "None") {
        compressionChoice = PHDFBatchEventsOutputer::CompressionChoice::kNone;
      } else {
        std::cout <<"Unknown compression choice "<<compressionChoiceName<<std::endl;
        return {};
      }

      auto batchSize = params.get<int>("batchSize", 1);
      return std::make_unique<PHDFBatchEventsOutputer>(*fileName, iNLanes, chunkSize, *compression, compressionLevel, compressionChoice, *serialization, batchSize, nEvents);
    }
  };

  Maker s_maker;
}
