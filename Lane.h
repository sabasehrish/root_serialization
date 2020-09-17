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
 Lane(unsigned int iIndex, std::unique_ptr<SourceBase> iSource, double iScaleFactor): source_(std::move(iSource)), index_{iIndex} {
    
    const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
    waiters_.reserve(source_->dataProducts().size());
    for( int ib = 0; ib< source_->dataProducts().size(); ++ib) {
      auto const& dp = source_->dataProducts()[ib];
      auto address = dp.address();
      if(eventAuxiliaryBranchName == dp.name()) {
	eventAuxReader_ = EventAuxReader(address);
      }
      
      waiters_.emplace_back(ib, iScaleFactor);
    }
    if(not eventAuxReader_) {
      eventAuxReader_ = EventAuxReader(nullptr);
    }
  }

  void processEventsAsync(std::atomic<long>& index, tbb::task_group& group, const OutputerBase& outputer) {
    doNextEvent(index, group,  outputer);
  }

  void setVerbose(bool iSet) { verbose_ = iSet; }

  std::vector<DataProductRetriever> const& dataProducts() const { return source_->dataProducts(); }

  std::chrono::microseconds sourceAccumulatedTime() const { return source_->accumulatedTime(); }
private:

  void processEventAsync(tbb::task_group& group, TaskHolder iCallback, const OutputerBase& outputer) { 
    //Process order: retrieve data product, do wait, serialize, do output, call iCallback 

    //std::cout <<"make process event task"<<std::endl;
    TaskHolder holder(group, 
		      make_functor_task([&outputer, this, callback=std::move(iCallback)]() {
			  outputer.outputAsync(this->index_, eventAuxReader_->doWork(),
					       std::move(callback));
			}));
    
    size_t index=0;
    for(auto& d: source_->dataProducts()) {
      TaskHolder waitH(group,
		       make_functor_task([&group, &outputer, index, &d, holder, this]() {
			  auto laneIndex = this->index_;
			  TaskHolder sH(group,
					make_functor_task([holder, laneIndex, &d, &outputer]() {
					    outputer.productReadyAsync(laneIndex, d, std::move(holder));
					  }));
			  auto& w = waiters_[index];
			  w.waitAsync(source_->dataProducts(),std::move(sH));
			 }) );

      d.getAsync(std::move(waitH));
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
  std::optional<EventAuxReader> eventAuxReader_;
  unsigned int index_;
  bool verbose_ = false;
};

#endif
