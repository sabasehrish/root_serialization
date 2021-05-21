#if !defined(TextDumpOutputer_h)
#define TextDumpOutputer_h

#include "OutputerBase.h"
#include "SerialTaskQueue.h"

namespace cce::tf {
class TextDumpOutputer final : public OutputerBase {
 public:
 TextDumpOutputer(bool perEventDump, bool summaryDump): eventCount_{0}, perEventDump_(perEventDump), summaryDump_(summaryDump) {}

  ~TextDumpOutputer() = default;
  
  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const&);
  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const&, TaskHolder iCallback) const;

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const;
  bool usesProductReadyAsync() const {return true;}

  void printSummary() const;
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
