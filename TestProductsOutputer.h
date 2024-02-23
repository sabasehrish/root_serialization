#if !defined(TestProductsOutputer_h)
#define TestProductsOutputer_h
#include "OutputerBase.h"

namespace cce::tf {

class TestProductsOutputer : public OutputerBase {
 public:
  TestProductsOutputer(unsigned int iNLanes, int nProducts);
  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const&) final;
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const&, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final;


  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;

  void printSummary() const final;
 private:
  std::vector<std::vector<DataProductRetriever> const*> retrieverPerLane_;
  int nProducts_;
};
}
#endif
