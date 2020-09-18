#if !defined(RepeatingRootSource_h)
#define RepeatingRootSource_h

#include <string>
#include <memory>
#include <optional>
#include <vector>

#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "EventAuxReader.h"

#include "SourceBase.h"
#include "TFile.h"

class TBranch;
class TTree;

class RepeatingRootDelayedRetriever : public DelayedProductRetriever {
  void getAsync(int index, TaskHolder) override {}
};

class RepeatingRootSource : public SourceBase {
public:
  RepeatingRootSource(std::string const& iName, unsigned int iNUniqueEvents, unsigned long long iNEvents);
  RepeatingRootSource(RepeatingRootSource&&) = default;
  RepeatingRootSource(RepeatingRootSource const&) = default;

  std::vector<DataProductRetriever>& dataProducts() final { return dataProducts_; }
  EventIdentifier eventIdentifier() final { return identifierPerEvent_[presentEventIndex_];}

  bool readEvent(long iEventIndex) final;

  struct BufferInfo {
    BufferInfo(void* iAddress, size_t iSize): address_{iAddress}, size_{iSize} {}
    void* address_;
    size_t size_;
  };

private:
  void fillBuffer(int iEntry, std::vector<BufferInfo>& , std::vector<TBranch*>&);

  unsigned int nUniqueEvents_;
  unsigned int presentEventIndex_;
  RepeatingRootDelayedRetriever delayedReader_;
  std::vector<DataProductRetriever> dataProducts_;
  std::vector<EventIdentifier> identifierPerEvent_;
  std::vector<std::vector<BufferInfo>> dataBuffersPerEvent_;
};
#endif
