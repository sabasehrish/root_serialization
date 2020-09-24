#if !defined(DummyOutputer_h)
#define DummyOutputer_h

#include "OutputerBase.h"

namespace cce::tf {
class DummyOutputer : public OutputerBase {
 public:
 DummyOutputer(bool iUseProductReadyAsync=false): use_{iUseProductReadyAsync} {}

  ~DummyOutputer() final {}
  
  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const&) final {}
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const&, TaskHolder iCallback) const final {}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final {}
  bool usesProductReadyAsync() const final {return use_;}

  void printSummary() const final {}
 private:
  bool use_;
};
}
#endif
