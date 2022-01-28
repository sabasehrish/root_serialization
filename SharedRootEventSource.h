#if !defined(SharedRootEventSource_h)
#define SharedRootEventSource_h

#include <string>
#include <memory>
#include <chrono>
#include <iostream>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "SharedSourceBase.h"
#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "SerialTaskQueue.h"
#include "DeserializeStrategy.h"
#include "pds_reading.h"


namespace cce::tf {
  class SharedRootEventDelayedRetriever : public DelayedProductRetriever {
    void getAsync(DataProductRetriever&, int index, TaskHolder) final {}
  };
  
  class SharedRootEventSource : public SharedSourceBase {
  public:
    SharedRootEventSource(unsigned int iNLanes, unsigned long long iNEvents, std::string const& iFileName);
    SharedRootEventSource(SharedRootEventSource&&) = delete;
    SharedRootEventSource(SharedRootEventSource const&) = delete;
    ~SharedRootEventSource() = default;

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
  std::unique_ptr<TFile> file_;
  TTree* eventsTree_;
  TBranch* eventsBranch_;
  TBranch* idBranch_;
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
    SharedRootEventDelayedRetriever delayedRetriever_;
    std::chrono::microseconds decompressTime_;
    std::chrono::microseconds deserializeTime_;
    ~LaneInfo();
  };

  std::vector<LaneInfo> laneInfos_;
  std::chrono::microseconds readTime_;
  };
}

#endif
