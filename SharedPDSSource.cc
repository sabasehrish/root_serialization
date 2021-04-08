#include "SharedPDSSource.h"
#include "TClass.h"

using namespace cce::tf;

SharedPDSSource::SharedPDSSource(unsigned int iNLanes, unsigned long long iNEvents, std::string const& iName) :
                 SharedSourceBase(iNEvents),
  file_{iName, std::ios_base::binary}
{
  auto productInfo = readFileHeader(file_, compression_);

  laneInfos_.reserve(iNLanes);
  for(unsigned int i = 0; i< iNLanes; ++i) {
    laneInfos_.emplace_back(productInfo);
  }
}

SharedPDSSource::LaneInfo::LaneInfo(std::vector<pds::ProductInfo> const& productInfo) {
  dataProducts_.reserve(productInfo.size());
  dataBuffers_.resize(productInfo.size(), nullptr);
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
    ++index;
  }
}

SharedPDSSource::LaneInfo::~LaneInfo() {
  auto it = dataProducts_.begin();
  for( void * b: dataBuffers_) {
    it->classType()->Destructor(b);
    ++it;
  }
}

size_t SharedPDSSource::numberOfDataProducts() const {
  return laneInfos_[0].dataProducts_.size();
}

std::vector<DataProductRetriever>& SharedPDSSource::dataProducts(unsigned int iLane, long iEventIndex) {
  return laneInfos_[iLane].dataProducts_;
}

EventIdentifier SharedPDSSource::eventIdentifier(unsigned int iLane, long iEventIndex) {
  return laneInfos_[iLane].eventID_;
}

void SharedPDSSource::readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder iTask) {
  queue_.push(*iTask.group(), [iLane, optTask = std::move(iTask), this]() mutable {
        std::vector<uint32_t> buffer;
        
        if(pds::readCompressedEventBuffer(file_, this->laneInfos_[iLane].eventID_, buffer)) {
          auto group = optTask.group();
          group->run([this, buffer=std::move(buffer), task = optTask.releaseToTaskHolder(), iLane]() {
              std::vector<uint32_t> uBuffer = pds::uncompressEventBuffer(this->compression_, buffer);
              pds::deserializeDataProducts(uBuffer.begin(), uBuffer.end(), this->laneInfos_[iLane].dataProducts_);
              //task.doneWaiting();
            });
        }
    });
}

std::chrono::microseconds SharedPDSSource::accumulatedTime() const {
  return std::chrono::microseconds{};
}
