#if !defined(FunctorTask_h)
#define FunctorTask_h
#include <type_traits>
#include <utility>
#include <memory>

#include "TaskBase.h"

template <typename F>
class FunctorTask : public TaskBase {
public:
  explicit FunctorTask(F f) : func_(std::move(f)) {}

  void execute() override {
    func_();
  };

private:
  std::remove_reference_t<F> func_;
};

template <typename F>
std::unique_ptr<FunctorTask<F>>  make_functor_task(F f) {
  return std::make_unique<FunctorTask<F>>(std::move(f));
}
#endif

