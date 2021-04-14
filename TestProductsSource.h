#if !defined(TestProductsSource_h)
#define TestProductsSource_h
#include "SharedSourceBase.h"
#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"

#include <vector>

namespace cce::tf {
class TestDelayedProductRetriever : public DelayedProductRetriever {
 public:
  TestDelayedProductRetriever(std::vector<int>*, std::vector<float>*);

  void getAsync(DataProductRetriever&, int index, TaskHolder iCallback) final;

  void setEventIndex(long);

  void** ints() { return reinterpret_cast<void**>(&ints_);}
  void** floats() { return reinterpret_cast<void**>(&floats_);}

 private:
  std::vector<int>* ints_;
  std::vector<float>* floats_;
  long eventIndex_;
};

class TestProductsSource : public SharedSourceBase {
 public:
  TestProductsSource(unsigned int iNLanes, unsigned long long iNEvents);

  size_t numberOfDataProducts() const final;
  std::vector<DataProductRetriever>& dataProducts(unsigned int iLane, long iEventIndex) final;
  EventIdentifier eventIdentifier(unsigned int iLane, long iEventIndex) final;

  virtual void printSummary() const final;

 private:
  void readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder) final;

  std::vector<TestDelayedProductRetriever> delayedPerLane_;
  std::vector<std::vector<DataProductRetriever>> retrieverPerLane_;
  std::vector<std::vector<int>> intsPerLane_;
  std::vector<std::vector<float>> floatsPerLane_;
};
}

#endif
