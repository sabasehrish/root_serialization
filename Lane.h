#if !defined(Lane_h)
#define Lane_h

#include <string>
#include <vector>
#include <atomic>
#include <utility>
#include <memory>

#include "tbb/task_group.h"
#include "tbb/parallel_for.h"

#include "SourceBase.h"
#include "SerializerWrapper.h"
#include "OutputerBase.h"
#include "Waiter.h"
#include "FunctorTask.h"

namespace cce::tf {
class Lane {
public:
 Lane(unsigned int iIndex, std::unique_ptr<SourceBase> iSource, double iScaleFactor): source_(std::move(iSource)), index_{iIndex} {
    if(iScaleFactor >=0.) {
      waiters_.reserve(source_->dataProducts().size());
      for( int ib = 0; ib< source_->dataProducts().size(); ++ib) {
	waiters_.emplace_back(ib, iScaleFactor);
      }
    }
  }

  void processEventsAsync(std::atomic<long>& index, tbb::task_group& group, const OutputerBase& outputer) {
    doNextEvent(index, group,  outputer);
  }

  void setVerbose(bool iSet) { verbose_ = iSet; }

  std::vector<DataProductRetriever> const& dataProducts() const { return source_->dataProducts(); }

  std::chrono::microseconds sourceAccumulatedTime() const { return source_->accumulatedTime(); }
private:

  TaskHolder makeWaiterTask(tbb::task_group& group, size_t index, TaskHolder holder) {
    if(waiters_.empty()) {
      return holder;
    } else {  
      return TaskHolder(group,
		     make_functor_task([index,  holder, this]() {
			 auto laneIndex = this->index_;
			 auto& w = waiters_[index];
			 w.waitAsync(source_->dataProducts(),std::move(holder));
		       }) );
    }
  }

  TaskHolder makeTaskForDataProduct(tbb::task_group& group, size_t index, DataProductRetriever& iDP, OutputerBase const& outputer, TaskHolder holder) {
      if(outputer.usesProductReadyAsync()) {
	auto laneIndex = this->index_;
	return makeWaiterTask(group, index,TaskHolder(group, 
					   make_functor_task([holder, laneIndex, &iDP, &outputer]() {
					       outputer.productReadyAsync(laneIndex, iDP, std::move(holder));
					     })));
      } else {
	return makeWaiterTask(group, index, holder);
      }
  }

  void processEventAsync(tbb::task_group& group, TaskHolder iCallback, const OutputerBase& outputer) { 
    //Process order: retrieve data product, do wait, serialize, do output, call iCallback 

    //std::cout <<"make process event task"<<std::endl;
    TaskHolder holder(group, 
		      make_functor_task([&outputer, this, callback=std::move(iCallback)]() {
			  outputer.outputAsync(this->index_, source_->eventIdentifier(),
					       std::move(callback));
			}));
    
    //NOTE: I once replaced with with a tbb::parallel_for but that made the code slower and did not
    // scale as well as the number of threads were increased.
    size_t index=0;
    for(auto& d: source_->dataProducts()) {
      d.getAsync(makeTaskForDataProduct(group, index,d, outputer, holder));
      ++index;
    }
  }

  void doNextEvent(std::atomic<long>& index, tbb::task_group& group,  const OutputerBase& outputer) {
    using namespace std::string_literals;
    auto i = ++index;
    i-=1;
    if(source_->gotoEvent(i)) {
      if(verbose_) {
        std::cout <<"event "+std::to_string(i)+"\n"<<std::flush;
      }
      
      //std::cout <<"make doNextEvent task"<<std::endl;
      TaskHolder recursiveTask(group, make_functor_task([this, &index, &group, &outputer]() {
	    doNextEvent(index, group, outputer);
	  }));
      processEventAsync(group, std::move(recursiveTask), outputer);
    }
  }

  std::unique_ptr<SourceBase> source_;
  //std::vector<SerializerWrapper> serializers_;
  std::vector<Waiter> waiters_;
  unsigned int index_;
  bool verbose_ = false;
};
}
#endif
