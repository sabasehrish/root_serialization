#if !defined(SerialRootTreeGetEntrySource_h)
#define SerialRootTreeGetEntrySource_h

#include <string>
#include <memory>
#include <optional>
#include <vector>

#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "EventAuxReader.h"

#include "SharedSourceBase.h"
#include "SerialTaskQueue.h"
#include "TFile.h"

class TBranch;
class TTree;

namespace cce::tf {
  class SerialRootTreeGetEntryDelayedRetriever : public DelayedProductRetriever {
  public:
    SerialRootTreeGetEntryDelayedRetriever(std::vector<TBranch*> const& iBranches) {setupBuffer(iBranches);}
    void getAsync(DataProductRetriever&, int index, TaskHolder) final;

    void setAddresses(std::vector<TBranch*> const& iBranches);
    
  private:
    void setupBuffer(std::vector<TBranch*> const& iBranches);
    std::vector<void*> buffers_;
  };

  class SerialRootTreeGetEntrySource : public SharedSourceBase {
  public:
    SerialRootTreeGetEntrySource(unsigned iNLanes, unsigned long long iNEvents, std::string const& iName);
    size_t numberOfDataProducts() const final {return dataProductsPerLane_[0].size();}

    std::vector<DataProductRetriever>& dataProducts(unsigned int iLane, long iEventIndex) final {
      return dataProductsPerLane_[iLane];
    }
    EventIdentifier eventIdentifier(unsigned int iLane, long iEventIndex) final {
      return identifiers_[iLane];
    }
    
    void printSummary() const final;
    std::chrono::microseconds accumulatedTime() const;
  private:
    void readEventAsync(unsigned int iLane, long iEventIndex,  OptionalTaskHolder) final;

    
    std::unique_ptr<TFile> file_;
    SerialTaskQueue queue_;
    TTree* events_;
    std::vector<TBranch*> branches_;
    long nEvents_;
    TBranch* eventAuxBranch_=nullptr;
    TBranch* eventIDBranch_=nullptr;
    EventAuxReader eventAuxReader_;
    std::chrono::microseconds accumulatedTime_;

    //per lane items
    std::vector<SerialRootTreeGetEntryDelayedRetriever> delayedReaders_;
    std::vector<EventIdentifier> identifiers_;
    std::vector<std::vector<DataProductRetriever>> dataProductsPerLane_;
  };
}

#endif
