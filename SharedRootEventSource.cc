#include "SharedRootEventSource.h"
#include "SourceFactory.h"
#include "Deserializer.h"
#include "UnrolledDeserializer.h"

#include "TClass.h"

using namespace cce::tf;

SharedRootEventSource::SharedRootEventSource(unsigned int iNLanes, unsigned long long iNEvents, std::string const& iName) :
                 SharedSourceBase(iNEvents),
                 file_{TFile::Open(iName.c_str())},
  readTime_{std::chrono::microseconds::zero()}
{

  //gDebug = 3;

  if(not file_) {
    std::cout <<"unknown file "<<iName<<std::endl;
    throw std::runtime_error("uknown file");
  }

  eventsTree_ = file_->Get<TTree>("Events");
  if(not eventsTree_) {
    std::cout <<"no Events TTree in file "<<iName<<std::endl;
    throw std::runtime_error("no Events TTree");
  }
  eventsBranch_ = eventsTree_->GetBranch("offsetsAndBlob");
  if( not eventsBranch_) {
    std::cout <<"no 'offsetsAndBlob' TBranch in 'Events' TTree in file "<<iName<<std::endl;
    throw std::runtime_error("no 'offsetsAndBlob' TBranch");
  }

  idBranch_ = eventsTree_->GetBranch("EventID");
  if(not idBranch_) {
    std::cout <<"no 'EventID' TBranch in 'Events' TTree in file "<<iName<<std::endl;
    throw std::runtime_error("no 'EventID' TBranch");
  }
   
  auto meta = file_->Get<TTree>("Meta");
  if(not meta) {
    std::cout <<"no 'Meta' TTree in file "<<iName<<std::endl;
    throw std::runtime_error("no 'Meta' TTree");
  }

  std::vector<std::pair<std::string, std::string>> typeAndNames;
  int objectSerializationUsed;
  std::string compression;

  {
    auto typeAndNamesBranch = meta->GetBranch("DataProducts");
    auto pTemp = &typeAndNames;
    typeAndNamesBranch->SetAddress(&pTemp);
    typeAndNamesBranch->GetEntry(0);

    //std::cout <<"typeAndNames.size() "<<typeAndNames.size()<<std::endl;
    //for(auto const& tn: typeAndNames) {
    //  std::cout <<"  "<<tn.first<<" "<<tn.second<<std::endl;
    //}
    
  }
  {
    auto serializerBranch = meta->GetBranch("objectSerializationUsed");
    serializerBranch->SetAddress(&objectSerializationUsed);
    serializerBranch->GetEntry(0);

    //std::cout <<"serialization "<<objectSerializationUsed<<std::endl;
  }
  {
    auto compressionBranch = meta->GetBranch("compressionAlgorithm");
    auto pTemp = &compression;
    compressionBranch->SetAddress(&pTemp);
    compressionBranch->GetEntry(0);
    //std::cout <<"compressionAlgorithm "<<compression<<std::endl;
  }

  pds::Serialization serialization;
  switch(objectSerializationUsed) {
  case static_cast<int>(pds::Serialization::kRoot):
    serialization = pds::Serialization::kRoot;
    break;
  case static_cast<int>(pds::Serialization::kRootUnrolled):
    serialization = pds::Serialization::kRootUnrolled;
    break;
  }

  if (compression == "None") {
    compression_ = pds::Compression::kNone;
  }else if (compression == "LZ4") {
    compression_ = pds::Compression::kLZ4;
  }else if (compression == "ZSTD") {
    compression_ = pds::Compression::kZSTD;
  } else {
    std::cout <<"Unknown compression algorithm '"<<compression<<"'"<<std::endl;
    throw std::runtime_error("unknown compression algorithm");
  }

  std::vector<pds::ProductInfo> productInfo;
  productInfo.reserve(typeAndNames.size());
  { 
    unsigned int index = 0;
    for( auto const& typeAndName: typeAndNames) {
      productInfo.emplace_back(typeAndName.second, index++, typeAndName.first);
    }
  }

  laneInfos_.reserve(iNLanes);
  for(unsigned int i = 0; i< iNLanes; ++i) {
    DeserializeStrategy strategy;
    switch(serialization) {
    case pds::Serialization::kRoot: { 
      strategy = DeserializeStrategy::make<DeserializeProxy<Deserializer>>(); break;
    }
    case pds::Serialization::kRootUnrolled: {
      strategy = DeserializeStrategy::make<DeserializeProxy<UnrolledDeserializer>>(); break;
    }
    }
    laneInfos_.emplace_back(productInfo, std::move(strategy));
  }


}

SharedRootEventSource::LaneInfo::LaneInfo(std::vector<pds::ProductInfo> const& productInfo, DeserializeStrategy deserialize):
  deserializers_{std::move(deserialize)},
  decompressTime_{std::chrono::microseconds::zero()},
  deserializeTime_{std::chrono::microseconds::zero()}
{
  dataProducts_.reserve(productInfo.size());
  dataBuffers_.resize(productInfo.size(), nullptr);
  deserializers_.reserve(productInfo.size());
  size_t index =0;
  for(auto const& pi : productInfo) {
    
    TClass* cls = TClass::GetClass(pi.className().c_str());
    dataBuffers_[index] = cls->New();
    assert(cls);
    dataProducts_.emplace_back(index,
			       &dataBuffers_[index],
                               pi.name(),
                               cls,
			       &delayedRetriever_);
    deserializers_.emplace_back(cls);
    ++index;
  }
}

SharedRootEventSource::LaneInfo::~LaneInfo() {
  auto it = dataProducts_.begin();
  for( void * b: dataBuffers_) {
    it->classType()->Destructor(b);
    ++it;
  }
}

size_t SharedRootEventSource::numberOfDataProducts() const {
  return laneInfos_[0].dataProducts_.size();
}

std::vector<DataProductRetriever>& SharedRootEventSource::dataProducts(unsigned int iLane, long iEventIndex) {
  return laneInfos_[iLane].dataProducts_;
}

EventIdentifier SharedRootEventSource::eventIdentifier(unsigned int iLane, long iEventIndex) {
  return laneInfos_[iLane].eventID_;
}

void SharedRootEventSource::readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder iTask) {
  queue_.push(*iTask.group(), [iLane, optTask = std::move(iTask), this, iEventIndex]() mutable {
      auto start = std::chrono::high_resolution_clock::now();
      if(iEventIndex < eventsTree_->GetEntries()) {
        std::pair<std::vector<uint32_t>, std::vector<char>> offsetsAndBuffer;
        auto pBuffer = &offsetsAndBuffer;
        eventsBranch_->SetAddress(&pBuffer);

        idBranch_->SetAddress(&this->laneInfos_[iLane].eventID_);
        eventsTree_->GetEntry(iEventIndex);
        {
          //auto const& id = this->laneInfos_[iLane].eventID_;
          //std::cout <<"event entry "<<iEventIndex<<std::endl;
          //std::cout <<"ID "<<id.run<<" "<<id.lumi<<" "<<id.event<<std::endl;
          //std::cout <<"buffer size "<<buffer.size()<<std::endl;
          //std::cout <<"offset size "<<offsets.size()<<std::endl;
          //for(auto b: buffer) {
          //  std::cout <<"  "<<b<<std::endl;
          //}
        }

        auto group = optTask.group();
        group->run([this, offsetsAndBuffer=std::move(offsetsAndBuffer), task = optTask.releaseToTaskHolder(), iLane]() {
            auto& laneInfo = this->laneInfos_[iLane];

            auto start = std::chrono::high_resolution_clock::now();
            std::vector<char> uBuffer = pds::uncompressBuffer(this->compression_, offsetsAndBuffer.second, offsetsAndBuffer.first.back());
            std::cout <<"uncompressed buffer size "<<uBuffer.size() <<std::endl;
            laneInfo.decompressTime_ += 
              std::chrono::duration_cast<decltype(laneInfo.decompressTime_)>(std::chrono::high_resolution_clock::now() - start);
            
            start = std::chrono::high_resolution_clock::now();
            //uBuffer.pop_back();
            pds::deserializeDataProducts(&(*uBuffer.begin()), &(*uBuffer.end()), 
                                         offsetsAndBuffer.first.begin(), offsetsAndBuffer.first.end(),
                                         laneInfo.dataProducts_, laneInfo.deserializers_);
            laneInfo.deserializeTime_ += 
              std::chrono::duration_cast<decltype(laneInfo.deserializeTime_)>(std::chrono::high_resolution_clock::now() - start);
          });
      }
      readTime_ +=std::chrono::duration_cast<decltype(readTime_)>(std::chrono::high_resolution_clock::now() - start);
    });
}

void SharedRootEventSource::printSummary() const {
  std::cout <<"\nSource:\n"
    "   read time: "<<readTime().count()<<"us\n"
    "   decompress time: "<<decompressTime().count()<<"us\n"
    "   deserialize time: "<<deserializeTime().count()<<"us\n"<<std::endl;
};

std::chrono::microseconds SharedRootEventSource::readTime() const {
  return readTime_;
}

std::chrono::microseconds SharedRootEventSource::decompressTime() const {
  auto time = std::chrono::microseconds::zero();
  for(auto const& l : laneInfos_) {
    time += l.decompressTime_;
  }
  return time;
}

std::chrono::microseconds SharedRootEventSource::deserializeTime() const {
  auto time = std::chrono::microseconds::zero();
  for(auto const& l : laneInfos_) {
    time += l.deserializeTime_;
  }
  return time;
}


namespace {
    class Maker : public SourceMakerBase {
  public:
    Maker(): SourceMakerBase("SharedRootEventSource") {}
      std::unique_ptr<SharedSourceBase> create(unsigned int iNLanes, unsigned long long iNEvents, ConfigurationParameters const& params) const final {
        auto fileName = params.get<std::string>("fileName");
        if(not fileName) {
          std::cout <<"no file name given\n";
          return {};
        }
        return std::make_unique<SharedRootEventSource>(iNLanes, iNEvents, *fileName);
    }
    };

  Maker s_maker;
}
