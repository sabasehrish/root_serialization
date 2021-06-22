#if !defined(SharedPDSSource_h)
#define SharedPDSSource_h

#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <fstream>

#include "SharedSourceBase.h"
#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "SerialTaskQueue.h"
#include "DeserializeStrategy.h"
#include "pds_reading.h"


namespace cce::tf {
  class SharedPDSDelayedRetriever : public DelayedProductRetriever {
    void getAsync(DataProductRetriever&, int index, TaskHolder) final {}
  };
  
  class SharedPDSSource : public SharedSourceBase {
  public:
    SharedPDSSource(unsigned int iNLanes, unsigned long long iNEvents, std::string const& iFileName);
    SharedPDSSource(SharedPDSSource&&) = delete;
    SharedPDSSource(SharedPDSSource const&) = delete;
    ~SharedPDSSource() = default;

  size_t numberOfDataProducts() const final;
  std::vector<DataProductRetriever>& dataProducts(unsigned int iLane, long iEventIndex) final;
  EventIdentifier eventIdentifier(unsigned int iLane, long iEventIndex) final;

  void printSummary() const final;
  private:
  
  void readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder) final;

  std::chrono::microseconds readTime() const;
  std::chrono::microseconds decompressTime() const;
  std::chrono::microseconds deserializeTime() const;

  pds::Compression compression_;
  std::ifstream file_;
  SerialTaskQueue queue_;

  struct LaneInfo {
    LaneInfo(std::vector<pds::ProductInfo> const&, DeserializeStrategy);

    LaneInfo(LaneInfo&&) = default;
    LaneInfo(LaneInfo const&) = delete;

    LaneInfo& operator=(LaneInfo&&) = default;
    LaneInfo& operator=(LaneInfo const&) = delete;

    EventIdentifier eventID_;
    std::vector<DataProductRetriever> dataProducts_;
    std::vector<void*> dataBuffers_;
    DeserializeStrategy deserializers_; //NOTE: could be shared between lanes?
    SharedPDSDelayedRetriever delayedRetriever_;
    std::chrono::microseconds decompressTime_;
    std::chrono::microseconds deserializeTime_;
    ~LaneInfo();
  };

  std::vector<LaneInfo> laneInfos_;
  std::chrono::microseconds readTime_;
  };
}

#endif
