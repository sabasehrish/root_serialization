#if !defined(SerialRNTupleRetrievers_h)
#define SerialRNTupleRetrievers_h

#include <string>
#include <atomic>
#include <vector>
#include <chrono>

#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"

#include "SerialTaskQueue.h"
#include "ROOT/RNTuple.hxx"
#include "ROOT/RNTupleReader.hxx"

namespace cce::tf {
  class SerialRNTuplePromptRetriever : public DelayedProductRetriever {
  public:
    SerialRNTuplePromptRetriever():
      accumulatedTime_{std::chrono::microseconds::zero().count()}{}
    SerialRNTuplePromptRetriever(SerialRNTuplePromptRetriever&&) = default;
    SerialRNTuplePromptRetriever(SerialRNTuplePromptRetriever const& iOther):
      accumulatedTime_(iOther.accumulatedTime_.load()) {}
    void getAsync(DataProductRetriever&, int index, TaskHolder) final;
    std::chrono::microseconds accumulatedTime() const { return std::chrono::microseconds{accumulatedTime_.load()};}

  private:
    std::atomic<std::chrono::microseconds::rep> accumulatedTime_;
  };

  class SerialRNTupleDelayedRetriever : public DelayedProductRetriever {
  public:
    SerialRNTupleDelayedRetriever(SerialTaskQueue* iQueue,
                                  ROOT::RNTupleReader& iReader,
                                  std::vector<std::string> const& iFieldIDs,
                                  std::vector<void*>* iProductPtrs):
      queue_(iQueue), addresses_(iProductPtrs), accumulatedTime_{std::chrono::microseconds::zero()}{
      fillViews(iReader, iFieldIDs);
    }

    void setEventIndex(std::uint64_t iIndex) { eventIndex_ = iIndex;}
    void getAsync(DataProductRetriever&, int index, TaskHolder) final;
    std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}

  private:
    void fillViews(ROOT::RNTupleReader& iReader, std::vector<std::string> const& iFieldIDs);
    SerialTaskQueue* queue_;
    std::vector<ROOT::RNTupleView<void>> views_;
    std::vector<void*>* addresses_;
    std::chrono::microseconds accumulatedTime_;
    std::uint64_t eventIndex_ = 0;
  };
}
#endif
