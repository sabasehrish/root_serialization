#if !defined(SerialRootSource_h)
#define SerialRootSource_h

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
  class SerialRootDelayedRetriever : public DelayedProductRetriever {
  public:
    SerialRootDelayedRetriever(SerialTaskQueue* iQueue,
                               std::vector<TBranch*>* iBranches):
    queue_(iQueue), branches_(iBranches),
      accumulatedTime_{std::chrono::microseconds::zero()}{}
    void getAsync(DataProductRetriever&, int index, TaskHolder) final;
    void setEntry(long iEntry) { entry_ = iEntry; }
    std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}

  private:
    SerialTaskQueue* queue_;
    std::vector<TBranch*>* branches_;
    std::chrono::microseconds accumulatedTime_;
    long entry_ = -1;
  };

  class SerialRootSource : public SharedSourceBase {
  public:
    SerialRootSource(unsigned iNLanes, unsigned long long iNEvents, std::string const& iName);
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
    std::optional<EventAuxReader> eventAuxReader_;
    std::chrono::microseconds accumulatedTime_;

    //per lane items
    std::vector<SerialRootDelayedRetriever> delayedReaders_;
    std::vector<EventIdentifier> identifiers_;
    std::vector<std::vector<DataProductRetriever>> dataProductsPerLane_;
  };
}

#endif
