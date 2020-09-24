#if !defined(ReplicatedSharedSource_h)
#define ReplicatedSharedSource_h

#include <vector>
#include "SharedSourceBase.h"

namespace cce::tf {
  template<typename S>
    class ReplicatedSharedSource : public SharedSourceBase {
  public:
    template<typename... Args>
      ReplicatedSharedSource(unsigned iNLanes, unsigned long long iNEvents, Args&&... iArgs):
    SharedSourceBase(iNEvents) {
      sources_.reserve(iNLanes);
      for(int i=0; i< iNLanes; ++i) {
        sources_.emplace_back(std::forward<Args>(iArgs)...);
      }
    }

    size_t numberOfDataProducts() const {return sources_[0].numberOfDataProducts();}

    std::vector<DataProductRetriever>& dataProducts(unsigned int iLane, long iEventIndex) final {
      return sources_[iLane].dataProducts();
    }
    EventIdentifier eventIdentifier(unsigned int iLane, long iEventIndex) final {
      return sources_[iLane].eventIdentifier();
    }

    std::chrono::microseconds accumulatedTime() const final {
      std::chrono::microseconds totalTime = std::chrono::microseconds::zero();
      for(auto const& s: sources_) {
        totalTime += s.accumulatedTime();
      }
      return totalTime;
    }

  private:
    void readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder iTask) final {
      if(sources_[iLane].gotoEvent(iEventIndex)) {
        iTask.runNow();
      }
    }
    std::vector<S> sources_;
  };
}

#endif
