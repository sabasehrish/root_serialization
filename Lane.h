#if !defined(Lane_h)
#define Lane_h

#include <vector>
#include <atomic>
#include <memory>

#include "tbb/task_group.h"

#include "SharedSourceBase.h"
#include "OutputerBase.h"
#include "Waiter.h"


namespace cce::tf {
class Lane {
public:
  Lane(unsigned int iIndex, SharedSourceBase* iSource, double iScaleFactor);

  void processEventsAsync(std::atomic<long>& index, tbb::task_group& group, const OutputerBase& outputer);

  void setVerbose(bool iSet) { verbose_ = iSet; }

  std::vector<DataProductRetriever> const& dataProducts() const { return source_->dataProducts(index_); }

  std::chrono::microseconds sourceAccumulatedTime() const { return source_->accumulatedTime(); }
private:

  TaskHolder makeWaiterTask(tbb::task_group& group, size_t index, TaskHolder holder) ;

  TaskHolder makeTaskForDataProduct(tbb::task_group& group, size_t index, DataProductRetriever& iDP, OutputerBase const& outputer, TaskHolder holder) ;

  void processEventAsync(tbb::task_group& group, TaskHolder iCallback, const OutputerBase& outputer);

  void doNextEvent(std::atomic<long>& index, tbb::task_group& group,  const OutputerBase& outputer);

  SharedSourceBase* source_;
  std::vector<Waiter> waiters_;
  unsigned int index_;
  bool verbose_ = false;
};
}
#endif
