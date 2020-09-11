#if !defined(SourceBase_h)
#define SourceBase_h

#include "DataProductRetriever.h"

#include <vector>
#include <chrono>

class SourceBase {

 public:
  explicit SourceBase(unsigned long long iNEvents): 
  maxNEvents_{iNEvents}, accumulatedTime_{std::chrono::microseconds::zero()} {}
  virtual ~SourceBase() {}

  virtual std::vector<DataProductRetriever>& dataProducts() = 0;

  bool gotoEvent(long iEventIndex);

  std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}

 private:
  virtual bool readEvent(long iEventIndex) = 0;
  
  const unsigned long long maxNEvents_;
  std::chrono::microseconds accumulatedTime_;
};

inline bool SourceBase::gotoEvent(long iEventIndex) {
  if(iEventIndex < maxNEvents_) {
    auto start = std::chrono::high_resolution_clock::now();
    auto more = readEvent(iEventIndex);
    accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
    return more;
  }
  return false;
}

#endif
