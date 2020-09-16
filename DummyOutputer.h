#if !defined(DummyOutputer_h)
#define DummyOutputer_h

#include "OutputerBase.h"

class DummyOutputer : public OutputerBase {
 public:
  ~DummyOutputer() final {}
  
  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const&) final {}
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const&, TaskHolder iCallback) const final {}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final {}

  void printSummary() const final {}
};
#endif
