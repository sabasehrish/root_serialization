#if !defined(OptionalTaskHolder_h)
#define OptionalTaskHolder_h

#include <memory>
#include "tbb/task_group.h"
#include "TaskBase.h"
#include "TaskHolder.h"

namespace cce::tf {
  class OptionalTaskHolder {
  public:
    OptionalTaskHolder() = delete;

    OptionalTaskHolder(tbb::task_group& iGroup, std::unique_ptr<TaskBase> iTask): 
    group_{&iGroup}, task_{std::move(iTask)} {
    }
    
    OptionalTaskHolder(OptionalTaskHolder&&) = default;
    
    OptionalTaskHolder& operator=(OptionalTaskHolder const&) = delete;
    OptionalTaskHolder& operator=(OptionalTaskHolder&&) = delete;
    
    tbb::task_group* group() { return group_;}

    TaskHolder releaseToTaskHolder() {
      return TaskHolder(*group_, std::move(task_));
    }
    
    void runNow() {
      auto t = std::move(task_);
      t->execute();
    }

  private:
    tbb::task_group* group_;
    std::unique_ptr<TaskBase> task_;
  };
}
#endif
