#if !defined(RNTupleOutputer_h)
#define RNTupleOutputer_h

#include <vector>
#include <string>
#include <iostream>
#include <cassert>

#define TBB_PREVIEW_TASK_GROUP_EXTENSIONS 1 // for task_group::defer
#include "tbb/task_group.h"
#include "tbb/concurrent_vector.h"

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializeStrategy.h"
#include "DataProductRetriever.h"
#include "summarize_serializers.h"
#include "SerialTaskQueue.h"

#include <ROOT/RNTuple.hxx>

namespace cce::tf {

class RNTupleOutputer : public OutputerBase {
 public:
  RNTupleOutputer(unsigned int iNLanes, std::string fileName, int iVerbose):
    entries_(iNLanes),
    fileName_(fileName),
    verbose_(iVerbose),
    collateTime_{std::chrono::microseconds::zero()},
    parallelTime_{0}
  { }

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;
  bool usesProductReadyAsync() const final {return true;}
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  void printSummary() const final;

private:
  struct EntryContainer {
    // std::unique_ptr<ROOT::Experimental::REntry> entry;
    std::vector<const ROOT::Experimental::Detail::RFieldBase*> fields;
    std::vector<void**> ptrs;
    // std::chrono::microseconds serializeTime{0};
    std::chrono::microseconds accumulatedTime() const { return std::chrono::microseconds::zero(); };
  };

  // Plan:
  // productReadyAsync() is threadsafe because entries_ is one per lane (also doesn't do anything right now)
  // outputAsync puts collateProducts() in collateQueue_
  // collateProducts() appends a new event to the RNTuple
  void collateProducts(EventIdentifier const& iEventID, EntryContainer& entry, TaskHolder iCallback) const;

  // configuration options
  const int verbose_;
  const std::string fileName_;

  // initialized in lane 0 setupForLane(), modified only in collateProducts()
  mutable std::unique_ptr<ROOT::Experimental::RNTupleWriter> ntuple_;

  // only modified by productReadyAsync()
  mutable std::vector<EntryContainer> entries_;

  // only modified in collateProducts()
  mutable SerialTaskQueue collateQueue_;
  mutable size_t eventGlobalOffset_{0};
  mutable std::chrono::microseconds collateTime_;

  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_;
};
}
#endif
