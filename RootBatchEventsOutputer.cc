#include "RootBatchEventsOutputer.h"
#include "OutputerFactory.h"
#include "ConfigurationParameters.h"
#include "UnrolledSerializerWrapper.h"
#include "SerializerWrapper.h"
#include "summarize_serializers.h"
#include "FunctorTask.h"
#include "lz4.h"
#include "zstd.h"
#include <iostream>
#include <cstring>
#include <set>

using namespace cce::tf;
using namespace cce::tf::pds;

RootBatchEventsOutputer::RootBatchEventsOutputer(std::string const& iFileName, unsigned int iNLanes, Compression iCompression, int iCompressionLevel, 
                                                 Serialization iSerialization, int autoFlush, int maxVirtualSize,
                                                 std::string const& iTFileCompression, int iTFileCompressionLevel,
                                                 uint32_t iBatchSize): 
  file_(iFileName.c_str(), "recreate", "", iTFileCompressionLevel),
  serializers_{iNLanes},
  eventBatches_{iNLanes},
  waitingEventsInBatch_(iNLanes),
  presentEventEntry_(0),
  batchSize_(iBatchSize),
  compression_{iCompression},
  compressionLevel_{iCompressionLevel},
  serialization_{iSerialization},
  serialTime_{std::chrono::microseconds::zero()},
  parallelTime_{0}
  {
    for(auto& v: waitingEventsInBatch_) {
      v.store(0);
    }
    for(auto& v:eventBatches_) {
      v.store(nullptr);
    }

    if(not iTFileCompression.empty()) {
      if(iTFileCompression == "ZLIB") {
        file_.SetCompressionAlgorithm(ROOT::kZLIB);
      } else if(iTFileCompression == "LZMA") {
        file_.SetCompressionAlgorithm(ROOT::kLZMA);
      } else if(iTFileCompression == "LZ4") {
        file_.SetCompressionAlgorithm(ROOT::kLZ4);
      } else if(iTFileCompression == "ZSTD") {
        file_.SetCompressionAlgorithm(ROOT::kZSTD);
      }else {
        std::cout <<"unknown compression algorithm "<<iTFileCompression<<std::endl;
        abort();
      }
    }


    //gDebug = 3;
    eventsTree_ = new TTree("Events", "", 0, &file_);

    eventsTree_->Branch("offsetsAndBlob", &offsetsAndBlob_);
    eventsTree_->Branch("EventIDs", &eventIDs_);

    //Turn off auto save
    eventsTree_->SetAutoSave(std::numeric_limits<Long64_t>::max());
    if(-1 != autoFlush) {
      eventsTree_->SetAutoFlush(autoFlush);
    }

    if(maxVirtualSize >= 0) {
      eventsTree_->SetMaxVirtualSize(static_cast<Long64_t>(maxVirtualSize));
    }
}

RootBatchEventsOutputer::~RootBatchEventsOutputer() = default;


void RootBatchEventsOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& s = serializers_[iLaneIndex];
  switch(serialization_) {
  case Serialization::kRoot:
    {   s = SerializeStrategy::make<SerializeProxy<SerializerWrapper>>(); break; }
  case Serialization::kRootUnrolled:
    {   s = SerializeStrategy::make<SerializeProxy<UnrolledSerializerWrapper>>(); break; }
  }
  s.reserve(iDPs.size());
  offsetsAndBlob_.first.resize(iDPs.size()+1,0);
  for(auto const& dp: iDPs) {
    s.emplace_back(dp.name(), dp.classType());
  }

  if(iLaneIndex == 0) {
    writeMetaData(s);
  }

}

void RootBatchEventsOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  auto& laneSerializers = serializers_[iLaneIndex];
  auto group = iCallback.group();
  laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
}

void RootBatchEventsOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto [offsets, buffer] = writeDataProductsToOutputBuffer(serializers_[iLaneIndex]);

  auto eventIndex = presentEventEntry_++;

  auto batchIndex = (eventIndex/batchSize_) % eventBatches_.size();
  //std::cout <<"eventIndex "<<eventIndex <<"  batchIndex "<< batchIndex<<std::endl;


  auto& batchContainerPtr = eventBatches_[batchIndex];
  auto batchContainer = batchContainerPtr.load();
  if( nullptr == batchContainer ) {
    auto newContainer = std::make_unique<std::vector<EventInfo>>(batchSize_);
    if(batchContainerPtr.compare_exchange_strong(batchContainer, newContainer.get())) {
      newContainer.release();
      batchContainer = batchContainerPtr.load();
    }
  }
  //std::cout <<"batchContainer "<<batchContainer<<std::endl;

  auto indexInBatch = eventIndex % (batchSize_);
  //std::cout <<"indexInBatch "<<indexInBatch<<std::endl;
  std::get<0>((*batchContainer)[indexInBatch]) = iEventID;
  std::get<1>((*batchContainer)[indexInBatch]) = std::move(offsets);
  std::get<2>((*batchContainer)[indexInBatch]) = std::move(buffer);

  assert(waitingEventsInBatch_[batchIndex].load() < batchSize_);

  auto waitingEvents = ++waitingEventsInBatch_[batchIndex];

  if(waitingEvents == batchSize_ ) {
    const_cast<RootBatchEventsOutputer*>(this)->finishBatchAsync(batchIndex, std::move(iCallback));
  }
  auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  parallelTime_ += time.count();
}

void RootBatchEventsOutputer::printSummary() const  {
  //make sure last batches are out

  auto start = std::chrono::high_resolution_clock::now();
  {
    tbb::task_group group;
    
    {
      TaskHolder th(group, make_functor_task([](){}));
      for( int index=0; index < waitingEventsInBatch_.size();++index) {
        if(0 != waitingEventsInBatch_[index].load()) {
          const_cast<RootBatchEventsOutputer*>(this)->finishBatchAsync(index, th);
        }
      }
    }
    
    group.wait();
  }
  file_.Write();
  auto writeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);


  std::cout <<"RootBatchEventsOutputer\n  total serial time at end event: "<<serialTime_.count()<<"us\n"
    "  total parallel time at end event: "<<parallelTime_.load()<<"us\n";


  start = std::chrono::high_resolution_clock::now();
  file_.Close();
  auto closeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

  std::cout << "  end of job file write time: "<<writeTime.count()<<"us\n";
  std::cout << "  end of job file close time: "<<closeTime.count()<<"us\n";
                                                                                         
  summarize_serializers(serializers_);
}

void RootBatchEventsOutputer::finishBatchAsync(unsigned int iBatchIndex, TaskHolder iCallback) {

  std::unique_ptr<std::vector<EventInfo>> batch(eventBatches_[iBatchIndex].exchange(nullptr));
  auto eventsInBatch = waitingEventsInBatch_[iBatchIndex].load();
  waitingEventsInBatch_[iBatchIndex] = 0;

  std::vector<EventIdentifier> batchEventIDs;
  batchEventIDs.reserve(batch->size());

  std::vector<uint32_t> batchOffsets;
  batchOffsets.reserve(batch->size() * (serializers_.size()+1));

  std::vector<char> batchBlob;

  int index = 0;
  for(auto& event: *batch) {
    if(index++ == eventsInBatch) {
      //batch was smaller than usual. Can happen at end of job
      break;
    }
    batchEventIDs.push_back(std::get<0>(event));

    auto& offsets = std::get<1>(event);
    std::copy(offsets.begin(), offsets.end(), std::back_inserter(batchOffsets));

    auto& blob = std::get<2>(event);
    std::copy(blob.begin(), blob.end(), std::back_inserter(batchBlob));

    //release memory
    blob = std::vector<char>();
  }

  auto compressedBlob = compressBuffer(batchBlob);
  batchBlob = std::vector<char>();

  
  queue_.push(*iCallback.group(), [this, eventIDs=std::move(batchEventIDs), offsets = std::move(batchOffsets), buffer = std::move(compressedBlob),  callback=std::move(iCallback)]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      const_cast<RootBatchEventsOutputer*>(this)->output(std::move(eventIDs), std::move(buffer), std::move(offsets));
        serialTime_ += std::chrono::duration_cast<decltype(serialTime_)>(std::chrono::high_resolution_clock::now() - start);
      callback.doneWaiting();
    });
  
}

void RootBatchEventsOutputer::output(std::vector<EventIdentifier> iEventIDs, std::vector<char>  iBuffer, std::vector<uint32_t> iOffsets) {

  eventIDs_ = std::move(iEventIDs);
  offsetsAndBlob_ = {std::move(iOffsets), std::move(iBuffer)};

  eventsTree_->Fill();

  offsetsAndBlob_ = {};
}

void RootBatchEventsOutputer::writeMetaData(SerializeStrategy const& iSerializers) {

  std::vector<std::pair<std::string, std::string>> typeAndNames;
  for(auto const& s: iSerializers) {
    typeAndNames.emplace_back(s.className(), s.name());
  }

  auto objectSerializationUsed = static_cast<int>(serialization_);


  std::string compression = pds::name(compression_);

  TTree* meta = new TTree("Meta", "File meta data", 0, &file_);
  meta->Branch("DataProducts",&typeAndNames, 0, 0);
  meta->Branch("objectSerializationUsed",&objectSerializationUsed);
  meta->Branch("compressionAlgorithm",&compression,0,0);

  meta->Fill();

}

std::pair<std::vector<uint32_t>, std::vector<char>> RootBatchEventsOutputer::writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const{
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
      //std::cout <<"  write: "<<s.name()<<std::endl;
      auto offset = offsets[index++];
      std::copy(s.blob().begin(), s.blob().end(), buffer.begin()+offset );
    }
    assert(buffer.size() == offsets[index]);
  }

  //std::cout <<"compressed "<<cSize<<" uncompressed "<<buffer.size()<<std::endl;
  //std::cout <<"compressed "<<(buffer.size())/float(cSize)<<std::endl;
  return {offsets,buffer};
}

std::vector<char> RootBatchEventsOutputer::compressBuffer(std::vector<char> const& iBuffer) const {
  return pds::compressBuffer(0, 0, compression_, compressionLevel_, iBuffer);
}

namespace {

  class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("RootBatchEventsOutputer") {}

    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params, int) const final {

      auto fileName = params.get<std::string>("fileName");
      if(not fileName) {
        std::cout <<"no file name given for RootBatchEventsOutputer\n";
        return {};
      }

      int compressionLevel = params.get<int>("compressionLevel", 18);

      auto compressionName = params.get<std::string>("compressionAlgorithm", "ZSTD");
      auto serializationName = params.get<std::string>("serializationAlgorithm", "ROOT");

      auto compression = pds::toCompression(compressionName);
      if(not compression) {
        std::cout <<"unknown compression "<<compressionName<<std::endl;
        return {};
      }

      auto serialization = pds::toSerialization(serializationName);
      if(not serialization) {
        std::cout <<"unknown serialization "<<serializationName<<std::endl;
        return {};
      }

      auto treeMaxVirtualSize =  params.get<int>("treeMaxVirtualSize", -1);
      auto autoFlush = params.get<int>("autoFlush", -1);

      auto fileLevelCompression = params.get<std::string>("tfileCompressionAlgorithm", "");
      auto fileLevelCompressionLevel = params.get<int>("tfileCompressionLevel",0);

      auto batchSize = params.get<int>("batchSize",1);
      
      return std::make_unique<RootBatchEventsOutputer>(*fileName,iNLanes, *compression, compressionLevel, *serialization, autoFlush, treeMaxVirtualSize, fileLevelCompression, fileLevelCompressionLevel, batchSize);
    }
    
  };

  Maker s_maker;
}


