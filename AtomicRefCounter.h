#if !defined(AtomicRefCounter_h)
#define AtomicRefCounter_h

#include <atomic>

namespace cce::tf {
  class AtomicRefCounter {
  public:
  explicit AtomicRefCounter(std::atomic<unsigned int>& iCounter): counter_(&iCounter) { ++(*counter_); }
    ~AtomicRefCounter() noexcept {
      if(counter_) {--(*counter_); }
    }
  AtomicRefCounter(AtomicRefCounter const& iOther): counter_(iOther.counter_) {
      if(counter_) {++(*counter_);}
    }
  AtomicRefCounter(AtomicRefCounter&& iOther): counter_(iOther.counter_) {
      iOther.counter_ = nullptr;
    }

    AtomicRefCounter& operator=(AtomicRefCounter const& iOther) {
      AtomicRefCounter tmp(iOther);
      std::swap(tmp.counter_, counter_);
      return *this;
    }

    AtomicRefCounter& operator=(AtomicRefCounter&& iOther) {
      AtomicRefCounter tmp(std::move(iOther));
      std::swap(tmp.counter_, counter_);
      return *this;
    }
  private:
    std::atomic<unsigned int>* counter_;
  };
}

#endif
