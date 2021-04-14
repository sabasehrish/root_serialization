#if !defined(EmptySource_h)
#define EmptySource_h

#include "SharedSourceBase.h"
#include <iostream>

namespace cce::tf {
class EmptySource : public SharedSourceBase {
 public:
  explicit EmptySource(unsigned long long iNEvents):
  SharedSourceBase(iNEvents) {}

  size_t numberOfDataProducts() const final {return 0;}
  std::vector<DataProductRetriever>& dataProducts(unsigned int iLane, long iEventIndex) final { return empty_;}
  EventIdentifier eventIdentifier(unsigned int iLane, long iEventIndex) final {
    return {1, 1, static_cast<unsigned long long>(iEventIndex+1)};
  }

  void printSummary() const final {std::cout <<"\nSource time: N/A"<<std::endl;}

 private:
  void readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder iHolder) final {
    iHolder.runNow();
  }

  std::vector<DataProductRetriever> empty_;

};
}
#endif
