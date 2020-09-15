#if !defined(RootSource_h)
#define RootSource_h

#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include "TClass.h"
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"

#include "DataProductRetriever.h"
#include "SourceBase.h"

class RootSource : public SourceBase {
public:
  RootSource(std::string const& iName, unsigned long long iNEvents);
  RootSource(RootSource&&) = default;
  RootSource(RootSource const&) = default;

  std::vector<DataProductRetriever>& dataProducts() final { return dataProducts_; }

  bool readEvent(long iEventIndex) final {
    if(iEventIndex<numberOfEvents()) {
      auto it = dataProducts_.begin();
      for(auto b: branches_) {
	(it++)->setSize( b->GetEntry(iEventIndex) );
      }
      return true;
    }
    return false;
  }

private:
  long numberOfEvents() { 
    return events_->GetEntriesFast();
  }

  std::unique_ptr<TFile> file_;
  TTree* events_;
  std::vector<DataProductRetriever> dataProducts_;
  std::vector<TBranch*> branches_;
};

inline RootSource::RootSource(std::string const& iName, unsigned long long iNEvents) :
                  SourceBase(iNEvents),
  file_{TFile::Open(iName.c_str())}
{
  events_ = file_->Get<TTree>("Events");
  auto l = events_->GetListOfBranches();

  dataProducts_.reserve(l->GetEntriesFast());
  branches_.reserve(l->GetEntriesFast());
  for( int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    b->SetupAddresses();
    TClass* class_ptr=nullptr;
    EDataType type;
    b->GetExpectedType(class_ptr,type);

    dataProducts_.emplace_back(reinterpret_cast<void**>(b->GetAddress()),
                               b->GetName(),
                               class_ptr);
    branches_.emplace_back(b);
  }
}
#endif
