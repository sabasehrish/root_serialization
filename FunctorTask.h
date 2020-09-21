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

  static void operator delete(void* ptr) {
  }

  static void* operator new(std::size_t sz, std::unique_ptr<char[]>& cache) {
    if(not cache) {
      cache.reset(new char[sz]);
    }
    return cache.get();
  }
  static void operator delete(void*, std::unique_ptr<char[]>&) {
    return;
  }


private:
  std::remove_reference_t<F> func_;
};

/*
template <typename F>
std::unique_ptr<FunctorTask<F>>  make_functor_task(F f) {
  return std::make_unique<FunctorTask<F>>(std::move(f));
}
*/

template <typename F>
std::unique_ptr<FunctorTask<F>>  make_functor_task(std::unique_ptr<char[]>&cache, F f) {
  return std::unique_ptr<FunctorTask<F>>(new(cache) FunctorTask<F>(std::move(f)));
}

#endif

