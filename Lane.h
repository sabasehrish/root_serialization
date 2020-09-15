#if !defined(Lane_h)
#define Lane_h

#include <string>
#include <vector>
#include <atomic>
#include <utility>
#include <optional>
#include <memory>

#include "tbb/task_group.h"

#include "SourceBase.h"
#include "SerializerWrapper.h"
#include "OutputerBase.h"
#include "Waiter.h"
#include "EventAuxReader.h"
#include "FunctorTask.h"

class Lane {
public:
 Lane(std::unique_ptr<SourceBase> iSource, double iScaleFactor): source_(std::move(iSource)) {
    
    const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
    serializers_.reserve(source_->dataProducts().size());
    waiters_.reserve(source_->dataProducts().size());
    for( int ib = 0; ib< source_->dataProducts().size(); ++ib) {
      auto const& dp = source_->dataProducts()[ib];
      auto address = dp.address();
      if(eventAuxiliaryBranchName == dp.name()) {
	eventAuxReader_ = EventAuxReader(address);
      }
      
      serializers_.emplace_back(dp.name(), address,dp.classType());
      waiters_.emplace_back(ib, iScaleFactor);
    }
  }

  void processEventsAsync(std::atomic<long>& index, tbb::task_group& group, const OutputerBase& outputer) {
    doNextEvent(index, group,  outputer);
  }

  std::vector<SerializerWrapper> const& serializers() const { return serializers_;}

  std::chrono::microseconds sourceAccumulatedTime() const { return source_->accumulatedTime(); }
private:

  void processEventAsync(tbb::task_group& group, TaskHolder iCallback, const OutputerBase& outputer) { 
    //std::cout <<"make process event task"<<std::endl;
    TaskHolder holder(group, 
		      make_functor_task([&outputer, this, callback=std::move(iCallback)]() {
			  outputer.outputAsync(eventAuxReader_->doWork(),
					       serializers_, std::move(callback));
			}));
    
    size_t index=0;
    for(auto& w: waiters_) {
      auto& s = serializers_[index];
      TaskHolder sH(group,
		    make_functor_task([holder,&group, &s]() {
			s.doWorkAsync(group, std::move(holder));
		      }));
      w.waitAsync(source_->dataProducts(),std::move(sH));
      ++index;
    }
  }

  void doNextEvent(std::atomic<long>& index, tbb::task_group& group,  const OutputerBase& outputer) {
    using namespace std::string_literals;
    auto i = ++index;
    i-=1;
    if(source_->gotoEvent(i)) {
      std::cout <<"event "+std::to_string(i)+"\n"<<std::flush;
      
      //std::cout <<"make doNextEvent task"<<std::endl;
      TaskHolder recursiveTask(group, make_functor_task([this, &index, &group, &outputer]() {
	    doNextEvent(index, group, outputer);
	  }));
      processEventAsync(group, std::move(recursiveTask), outputer);
    }
  }

  std::unique_ptr<SourceBase> source_;
  std::vector<SerializerWrapper> serializers_;
  std::vector<Waiter> waiters_;
  std::optional<EventAuxReader> eventAuxReader_;
};

#endif
