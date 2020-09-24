#if !defined(TaskBase_h)
#define TaskBase_h

#include <atomic>

namespace cce::tf {
class TaskBase {
public:
  TaskBase() = default;
  virtual ~TaskBase() {
  }
  virtual void execute() = 0;

  void increment_ref_count() { ++refCount_;}
  bool decrement_ref_count() { return 0 == --refCount_;}
private:
  std::atomic<unsigned int> refCount_{0};
};
}
#endif
