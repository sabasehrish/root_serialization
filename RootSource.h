#if !defined(RootSource_h)
#define RootSource_h

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

class RootDelayedRetriever : public DelayedProductRetriever {
  void getAsync(int index, TaskHolder) override {}
};

class RootSource : public SourceBase {
public:
  RootSource(std::string const& iName, unsigned long long iNEvents);
  RootSource(RootSource&&) = default;
  RootSource(RootSource const&) = default;

  std::vector<DataProductRetriever>& dataProducts() final { return dataProducts_; }
  EventIdentifier eventIdentifier() final { return eventAuxReader_->doWork();}

  bool readEvent(long iEventIndex) final;

private:
  long numberOfEvents();

  std::unique_ptr<TFile> file_;
  TTree* events_;
  RootDelayedRetriever delayedReader_;
  std::optional<EventAuxReader> eventAuxReader_;
  std::vector<DataProductRetriever> dataProducts_;
  std::vector<TBranch*> branches_;
};
#endif
