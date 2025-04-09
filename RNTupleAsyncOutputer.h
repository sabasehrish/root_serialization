#if !defined(RNTupleAsyncOutputer_h)
#define RNTupleAsyncOutputer_h

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializeStrategy.h"
#include "DataProductRetriever.h"
#include "summarize_serializers.h"
#include "SerialTaskQueue.h"
#include "RNTupleOutputerConfig.h"
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleParallelWriter.hxx>
#include <ROOT/RNTupleFillContext.hxx>

namespace cce::tf {


class RNTupleAsyncOutputer : public OutputerBase {
 public:
  RNTupleAsyncOutputer(std::string const& fileName, unsigned int iNLanes, RNTupleOutputerConfig const&);

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;
  bool usesProductReadyAsync() const final {return true;}
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  void printSummary() const final;

private:
  struct LaneContainer {
    // std::unique_ptr<ROOT::REntry> entry;
    std::vector<DataProductRetriever> const* retrievers;
    std::shared_ptr<ROOT::Experimental::RNTupleFillContext> fillContext;
  };

  decltype(std::chrono::high_resolution_clock::now()) startClock() const;
  void stopClock(decltype(std::chrono::high_resolution_clock::now()) const&) const;

  //returns non-nullptr if context must be flushed
  ROOT::Experimental::RNTupleFillContext* fillProducts(EventIdentifier const& iEventID, LaneContainer const& entry) const;

  // configuration options
  const std::string fileName_;
  const RNTupleOutputerConfig config_;

  mutable SerialTaskQueue queue_;

  // initialized in lane 0 setupForLane(), modified only in collateProducts()
  mutable std::unique_ptr<ROOT::Experimental::RNTupleParallelWriter> ntuple_;

  //identifiers used to specify which field to use
  std::vector<std::string> fieldIDs_;
  
  mutable std::vector<LaneContainer> laneInfos_;

  // only modified in collateProducts()
  mutable std::atomic<size_t> eventGlobalOffset_{0};
  mutable std::chrono::microseconds collateTime_;
  bool hasEventAuxiliaryBranch_ = true;

  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_ = 0;
  mutable std::atomic<std::chrono::microseconds::rep> wallclockTime_ = 0;
  mutable std::chrono::microseconds::rep flushClusterTime_ = 0;
  mutable std::mutex wallclockMutex_;
  mutable decltype(std::chrono::high_resolution_clock::now()) wallclockStartTime_;
  mutable unsigned int nConcurrentCalls_=0;

};
}
#endif
