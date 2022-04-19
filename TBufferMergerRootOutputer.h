#if !defined(TBufferMergerRootOutputer_h)
#define TBufferMergerRootOutputer_h

#include <vector>
#include <string>
#include <cstdint>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "DataProductRetriever.h"

#include "SerialTaskQueue.h"

#include "ROOT/TBufferMerger.hxx"

class TBranch;
class TTree;

namespace cce::tf {

class TBufferMergerRootOutputer :public OutputerBase {
 public:
  struct Config {
    constexpr static int kDefaultAutoFlush = -30000000;
    int splitLevel_=99;
    int compressionLevel_=9;
    std::string compressionAlgorithm_="";
    int basketSize_=16384;
    int cacheSize_=0;
    int treeMaxVirtualSize_=-1;
    int autoFlush_=kDefaultAutoFlush; //This is ROOT's default value
    bool concurrentWrite = false;
  };

  TBufferMergerRootOutputer(std::string const& iFileName, unsigned int iNLanes, Config const&);
  ~TBufferMergerRootOutputer();

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return false;}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  
  void printSummary() const final;


private:
  struct PerLane {
    std::shared_ptr<ROOT::Experimental::TBufferMergerFile> file_;
    TTree* eventTree_;
    std::vector<TBranch*> branches_;
    std::vector<DataProductRetriever> const* retrievers_;
    std::chrono::microseconds accumulatedFillTime_;
    std::chrono::microseconds accumulatedWriteTime_;
    int nBytesWrittenSinceLastWrite_ = 0;
    int nEventsSinceWrite_ = 0;
    std::atomic<bool> shouldWrite_ = false;
  };
  
  void write(unsigned int iLaneIndex, TaskHolder iCallback);
  void writeWhenBytesFull(unsigned int iLaneIndex);
  void writeWhenEnoughEvents(unsigned int iLaneIndex);
  ROOT::Experimental::TBufferMerger buffer_;
  SerialTaskQueue queue_;
  std::vector<PerLane> lanes_;
  const int basketSize_;
  const int splitLevel_;
  const int treeMaxVirtualSize_;
  const int autoFlush_;
  std::atomic<int> numberEventsSinceLastWrite_;
  bool concurrentWrite_;
};
}
#endif
