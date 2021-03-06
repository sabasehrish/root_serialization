#if !defined(SerializeOutputer_h)
#define SerializeOutputer_h

#include <vector>
#include <string>
#include <iostream>
#include <cassert>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializerWrapper.h"
#include "DataProductRetriever.h"
#include "summarize_serializers.h"

#include "SerialTaskQueue.h"

namespace cce::tf {
class SerializeOutputer :public OutputerBase {
 public:
 SerializeOutputer(unsigned int iLaneIndex, bool iVerbose): serializers_(iLaneIndex), verbose_(iVerbose) {}
  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final {
    auto& s = serializers_[iLaneIndex];
    s.reserve(iDPs.size());
    for(auto const& dp: iDPs) {
      s.emplace_back(dp.name(), dp.classType());
    }
  }

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final {
    assert(iLaneIndex < serializers_.size());
    auto& laneSerializers = serializers_[iLaneIndex];
    auto group = iCallback.group();
    assert(iDataProduct.index() < laneSerializers.size() );
    laneSerializers[iDataProduct.index()].doWorkAsync(*group, iDataProduct.address(), std::move(iCallback));
  }

  bool usesProductReadyAsync() const final {return true; }

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final {
    queue_.push(*iCallback.group(), [this, iEventID, iLaneIndex, callback=std::move(iCallback)]() mutable {
	output(iEventID, serializers_[iLaneIndex]);
	callback.doneWaiting();
      });
  }
  
  void printSummary() const final {
    summarize_serializers(serializers_);
  }

 private:
  void output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) const {
    using namespace std::string_literals;
    if(verbose_) {
      std::cout <<"   run:"s+std::to_string(iEventID.run)+" lumi:"s+std::to_string(iEventID.lumi)+" event:"s+std::to_string(iEventID.event)+"\n"<<std::flush;
    }
    /*
    for(auto& s: iSerializers) {
      std::cout<<"   "s+std::string(s.name())+" size "+std::to_string(s.blob().size())+"\n" <<std::flush;
    }
    */
  }
private:
  mutable std::vector<std::vector<SerializerWrapper>> serializers_;
  mutable SerialTaskQueue queue_;
  bool verbose_;
};
}
#endif
