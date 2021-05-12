#if !defined(RepeatingRootSource_h)
#define RepeatingRootSource_h

#include <string>
#include <memory>
#include <optional>
#include <vector>

#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "EventAuxReader.h"

#include "SharedSourceBase.h"
#include "TFile.h"

class TBranch;
class TTree;

namespace cce::tf {
class RepeatingRootDelayedRetriever : public DelayedProductRetriever {
  void getAsync(DataProductRetriever&, int index, TaskHolder) override {}
};

class RepeatingRootSource : public SharedSourceBase {
public:
  RepeatingRootSource(std::string const& iName, unsigned int iNUniqueEvents, unsigned int iNLanes, unsigned long long iNEvents);
  RepeatingRootSource(RepeatingRootSource&&) = default;
  RepeatingRootSource(RepeatingRootSource const&) = default;
  ~RepeatingRootSource() final;

  size_t numberOfDataProducts() const final {return dataProductsPerLane_[0].size();}
  std::vector<DataProductRetriever>& dataProducts(unsigned int iLane, long iEventIndex ) final { return dataProductsPerLane_[iLane]; }
  EventIdentifier eventIdentifier(unsigned int iLane, long iEventIndex) final { return identifierPerEvent_[iEventIndex % nUniqueEvents_];}

  void printSummary() const final;
  std::chrono::microseconds accumulatedTime() const { return std::chrono::microseconds(accumulatedTime_.load());}

  void readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder) final;

  struct BufferInfo {
    BufferInfo(void* iAddress, size_t iSize): address_{iAddress}, size_{iSize} {}
    void* address_;
    size_t size_;
  };

private:
  void fillBuffer(int iEntry, std::vector<BufferInfo>& , std::vector<TBranch*>&);

  unsigned int nUniqueEvents_;
  RepeatingRootDelayedRetriever delayedReader_;
  std::vector<std::vector<DataProductRetriever>> dataProductsPerLane_;
  std::vector<std::vector<BufferInfo>> dataBuffersPerEvent_;
  std::vector<EventIdentifier> identifierPerEvent_;
  std::atomic<std::chrono::microseconds::rep> accumulatedTime_;
};
}
#endif
