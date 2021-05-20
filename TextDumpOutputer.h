#if !defined(TextDumpOutputer_h)
#define TextDumpOutputer_h

#include "OutputerBase.h"
#include "SerialTaskQueue.h"

namespace cce::tf {
class TextDumpOutputer : public OutputerBase {
 public:
 TextDumpOutputer(bool perEventDump, bool summaryDump): perEventDump_(perEventDump), summaryDump_(summaryDump), eventCount_{0} {}

  ~TextDumpOutputer() final {}
  
  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const&) final;
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const&, TaskHolder iCallback) const final;

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return true;}

  void printSummary() const final;
 private:
    mutable SerialTaskQueue queue_;
    std::vector<std::string> productNames_;
    mutable std::vector<std::atomic<unsigned long long>> productSizes_;
    mutable std::atomic<unsigned long> eventCount_;
    bool perEventDump_;
    bool summaryDump_;
};
}
#endif
