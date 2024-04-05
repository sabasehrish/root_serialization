#if !defined(RNTupleTFileOutputer_h)
#define RNTupleTFileOutputer_h

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <chrono>
#include <memory>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializeStrategy.h"
#include "DataProductRetriever.h"
#include "summarize_serializers.h"
#include "SerialTaskQueue.h"
#include "RNTupleOutputerConfig.h"
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include "TFile.h"

namespace cce::tf {


class RNTupleTFileOutputer : public OutputerBase {
 public:
  RNTupleTFileOutputer(std::string const& fileName, unsigned int iNLanes, RNTupleOutputerConfig const&);

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;
  bool usesProductReadyAsync() const final {return true;}
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  void printSummary() const final;

private:
  struct EntryContainer {
    // std::unique_ptr<ROOT::Experimental::REntry> entry;
    std::vector<DataProductRetriever> const* retrievers;
  };

  // Plan:
  // productReadyAsync() is threadsafe because entries_ is one per lane (also doesn't do anything right now)
  // outputAsync puts collateProducts() in collateQueue_
  // collateProducts() appends a new event to the RNTuple
  void collateProducts(EventIdentifier const& iEventID, EntryContainer const& entry, TaskHolder iCallback) const;

  // configuration options
  const std::string fileName_;
  const RNTupleOutputerConfig config_;

  TFile file_;
  // initialized in lane 0 setupForLane(), modified only in collateProducts()
  mutable std::unique_ptr<ROOT::Experimental::RNTupleWriter> ntuple_;

  //identifiers used to specify which field to use
  std::vector<std::string> fieldIDs_;
  
  std::vector<EntryContainer> entries_;

  mutable SerialTaskQueue collateQueue_;
  
  // only modified in collateProducts()
  mutable size_t eventGlobalOffset_{0};
  mutable std::chrono::microseconds collateTime_;
  mutable std::shared_ptr<EventIdentifier> id_;

  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_;


};
}
#endif
