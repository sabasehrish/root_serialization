#include "RootEventOutputer.h"
#include "OutputerFactory.h"
#include "ConfigurationParameters.h"
#include "UnrolledSerializerWrapper.h"
#include "SerializerWrapper.h"
#include "summarize_serializers.h"
#include "lz4.h"
#include "zstd.h"
#include <iostream>
#include <cstring>
#include <set>

using namespace cce::tf;
using namespace cce::tf::pds;

RootEventOutputer::RootEventOutputer(std::string const& iFileName, unsigned int iNLanes, Compression iCompression, int iCompressionLevel, 
                                     Serialization iSerialization, int autoFlush, int maxVirtualSize ): 
  file_(iFileName.c_str(), "recreate", "", 0),
  serializers_{std::size_t(iNLanes)},
  compression_{iCompression},
  compressionLevel_{iCompressionLevel},
  serialization_{iSerialization},
  serialTime_{std::chrono::microseconds::zero()},
  parallelTime_{0}
  {
    //gDebug = 3;
    eventsTree_ = new TTree("Events", "", 0, &file_);

    eventsTree_->Branch("blob", &eventBlob_);
    eventsTree_->Branch("EventID", &eventID_, "run/i:lumi/i:event/l");

    //Turn off auto save
    eventsTree_->SetAutoSave(std::numeric_limits<Long64_t>::max());
    if(-1 != autoFlush) {
      eventsTree_->SetAutoFlush(autoFlush);
    }

    if(maxVirtualSize >= 0) {
      eventsTree_->SetMaxVirtualSize(static_cast<Long64_t>(maxVirtualSize));
    }
}

RootEventOutputer::~RootEventOutputer() {
}


void RootEventOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& s = serializers_[iLaneIndex];
  switch(serialization_) {
  case Serialization::kRoot:
    {   s = SerializeStrategy::make<SerializeProxy<SerializerWrapper>>(); break; }
  case Serialization::kRootUnrolled:
    {   s = SerializeStrategy::make<SerializeProxy<UnrolledSerializerWrapper>>(); break; }
  }
  s.reserve(iDPs.size());
  for(auto const& dp: iDPs) {
    s.emplace_back(dp.name(), dp.classType());
  }
}

void RootEventOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  auto& laneSerializers = serializers_[iLaneIndex];
  auto group = iCallback.group();
  laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
}

void RootEventOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto tempBuffer = std::make_unique<std::vector<uint32_t>>(writeDataProductsToOutputBuffer(serializers_[iLaneIndex]));
  queue_.push(*iCallback.group(), [this, iEventID, iLaneIndex, callback=std::move(iCallback), buffer=std::move(tempBuffer)]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      const_cast<RootEventOutputer*>(this)->output(iEventID, serializers_[iLaneIndex],*buffer);
      buffer.reset();
        serialTime_ += std::chrono::duration_cast<decltype(serialTime_)>(std::chrono::high_resolution_clock::now() - start);
      callback.doneWaiting();
    });
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    parallelTime_ += time.count();
}

void RootEventOutputer::printSummary() const  {
  std::cout <<"RootEventOutputer\n  total serial time at end event: "<<serialTime_.count()<<"us\n"
    "  total parallel time at end event: "<<parallelTime_.load()<<"us\n";

  auto start = std::chrono::high_resolution_clock::now();
  file_.Write();
  auto writeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

  start = std::chrono::high_resolution_clock::now();
  file_.Close();
  auto closeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

  std::cout << "  end of job file write time: "<<writeTime.count()<<"us\n";
  std::cout << "  end of job file close time: "<<closeTime.count()<<"us\n";
                                                                                         
  summarize_serializers(serializers_);
}



void RootEventOutputer::output(EventIdentifier const& iEventID, SerializeStrategy const& iSerializers, std::vector<uint32_t>const& iBuffer) {
  if(firstTime_) {
    writeMetaData(iSerializers);
    firstTime_ = false;
  }
  //using namespace std::string_literals;
  
  //std::cout <<"   run:"s+std::to_string(iEventID.run)+" lumi:"s+std::to_string(iEventID.lumi)+" event:"s+std::to_string(iEventID.event)+"\n"<<std::flush;
  
  //writeEventID(iEventID);
  eventID_ = iEventID;
  eventBlob_ = std::move(iBuffer);
  //std::cout <<"Event "<<eventID_.run<<" "<<eventID_.lumi<<" "<<eventID_.event<<std::endl;
  //std::cout <<"buffer size "<<eventBlob_.size();
  //for(auto b: eventBlob_) {
  //  std::cout <<"   "<<b<<std::endl;
  //}
  eventsTree_->Fill();
  /*
    for(auto& s: iSerializers) {
    std::cout<<"   "s+s.name()+" size "+std::to_string(s.blob().size())+"\n" <<std::flush;
    }
  */
}

void RootEventOutputer::writeMetaData(SerializeStrategy const& iSerializers) {

  std::vector<std::pair<std::string, std::string>> typeAndNames;
  for(auto const& s: iSerializers) {
    std::string type(s.className());
    std::string name(s.name());
    typeAndNames.emplace_back(std::move(type), std::move(name));
  }

  int objectSerializationUsed = static_cast<std::underlying_type_t<Serialization>>(serialization_);


  std::string compression = pds::name(compression_);

  TTree* meta = new TTree("Meta", "File meta data", 0, &file_);
  meta->Branch("DataProducts",&typeAndNames, 0, 0);
  meta->Branch("objectSerializationUsed",&objectSerializationUsed);
  meta->Branch("compressionAlgorithm",&compression,0,0);

  meta->Fill();

}

std::vector<uint32_t> RootEventOutputer::writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const{
  //Calculate buffer size needed
  uint32_t bufferSize = 0;
  for(auto const& s: iSerializers) {
    bufferSize +=1+1;
    auto const blobSize = s.blob().size();
    bufferSize += bytesToWords(blobSize); //handles padding
  }
  //initialize with 0
  std::vector<uint32_t> buffer(size_t(bufferSize), 0);
  
  {
    uint32_t bufferIndex = 0;
    uint32_t dataProductIndex = 0;
    for(auto const& s: iSerializers) {
      //std::cout <<"  write: "<<s.name()<<std::endl;
      buffer[bufferIndex++]=dataProductIndex++;
      auto const blobSize = s.blob().size();
      uint32_t sizeInWords = bytesToWords(blobSize);
      buffer[bufferIndex++]=sizeInWords;
      std::copy(s.blob().begin(), s.blob().end(), reinterpret_cast<char*>( &(*(buffer.begin()+bufferIndex)) ) );
      bufferIndex += sizeInWords;
    }
    assert(buffer.size() == bufferIndex);
  }

  //will prepend one extra space at the beginning
  auto [cBuffer, cSize] = compressBuffer(buffer);

  //std::cout <<"compressed "<<cSize<<" uncompressed "<<buffer.size()*4<<std::endl;
  //std::cout <<"compressed "<<(buffer.size()*4)/float(cSize)<<std::endl;
  uint32_t const recordSize = bytesToWords(cSize)+1;
  //Record the actual number of bytes used in the last word of the compression buffer in the lowest
  // 2 bits of the word
  cBuffer[0] = buffer.size()*4 + (cSize % 4);
  if(cBuffer.size() != recordSize+1) {
    std::cout <<"BAD BUFFER SIZE: want: "<<recordSize+1<<" got "<<cBuffer.size()<<std::endl;
  }
  assert(cBuffer.size() == recordSize+1);
  return cBuffer;
}

std::pair<std::vector<uint32_t>, int> RootEventOutputer::compressBuffer(std::vector<uint32_t> const& iBuffer) const {
  return pds::compressBuffer(1, 0, compression_, compressionLevel_, iBuffer);
}

namespace {

  class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("RootEventOutputer") {}

    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {

      auto fileName = params.get<std::string>("fileName");
      if(not fileName) {
        std::cout <<"no file name given for RootEventOutputer\n";
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
      
      return std::make_unique<RootEventOutputer>(*fileName,iNLanes, *compression, compressionLevel, *serialization, autoFlush, treeMaxVirtualSize);
    }
    
  };

  Maker s_maker;
}


