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

namespace cce::tf {
class RootDelayedRetriever : public DelayedProductRetriever {
  void getAsync(DataProductRetriever&, int index, TaskHolder) override {}
};

class RootSource : public SourceBase {
public:
  RootSource(std::string const& iName);
  RootSource(RootSource&&) = default;
  RootSource(RootSource const&) = default;

  size_t numberOfDataProducts() const final {return dataProducts_.size();}
  std::vector<DataProductRetriever>& dataProducts() final { return dataProducts_; }
  EventIdentifier eventIdentifier() final;

  bool readEvent(long iEventIndex) final;

private:
  long numberOfEvents();

  std::unique_ptr<TFile> file_;
  TTree* events_;
  RootDelayedRetriever delayedReader_;
  EventAuxReader eventAuxReader_;
  TBranch* eventIDBranch_ = nullptr;
  EventIdentifier id_;
  std::vector<DataProductRetriever> dataProducts_;
  std::vector<TBranch*> branches_;
};
}
#endif
