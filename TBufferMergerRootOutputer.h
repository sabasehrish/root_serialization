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
    int splitLevel_=99;
    int compressionLevel_=9;
    std::string compressionAlgorithm_="";
    int basketSize_=16384;
    int treeMaxVirtualSize_=-1;
    int autoFlush_=-1;
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
    std::chrono::microseconds accumulatedTime_;
  };
  
  void write(unsigned int iLaneIndex);
  ROOT::Experimental::TBufferMerger buffer_;
  std::vector<PerLane> lanes_;
  const int basketSize_;
  const int splitLevel_;
  const int treeMaxVirtualSize_;
  const int autoFlush_;
};
}
#endif
