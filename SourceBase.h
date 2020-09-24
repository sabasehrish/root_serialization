#if !defined(SourceBase_h)
#define SourceBase_h

#include "DataProductRetriever.h"
#include "EventIdentifier.h"

#include <vector>
#include <chrono>

namespace cce::tf {
class SourceBase {

 public:
  explicit SourceBase(): 
  accumulatedTime_{std::chrono::microseconds::zero()} {}
  virtual ~SourceBase() {}

  virtual size_t numberOfDataProducts() const = 0;
  virtual std::vector<DataProductRetriever>& dataProducts() = 0;
  virtual EventIdentifier eventIdentifier() = 0;

  bool gotoEvent(long iEventIndex);

  std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}

 private:
  virtual bool readEvent(long iEventIndex) = 0;
  
  std::chrono::microseconds accumulatedTime_;
};

inline bool SourceBase::gotoEvent(long iEventIndex) {
  auto start = std::chrono::high_resolution_clock::now();
  auto more = readEvent(iEventIndex);
  accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
  return more;
}
}
#endif
