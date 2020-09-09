#if !defined(Source_h)
#define Source_h

#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include "TClass.h"
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"

#include "DataProductRetriever.h"

class Source {
public:
  Source(std::string const& iName, unsigned long long iNEvents);
  Source(Source&&) = default;
  Source(Source const&) = default;

  std::vector<DataProductRetriever>& dataProducts() { return dataProducts_; }

  bool gotoEvent(long iEventIndex) {
    if(iEventIndex<numberOfEvents() and iEventIndex < maxNEvents_) {
      auto start = std::chrono::high_resolution_clock::now();
      events_->GetEntry(iEventIndex);
      accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
      return true;
    }
    return false;
  }

  std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}
private:
  long numberOfEvents() { 
    return events_->GetEntriesFast();
  }

  std::unique_ptr<TFile> file_;
  const unsigned long long maxNEvents_;
  TTree* events_;
  std::vector<DataProductRetriever> dataProducts_;
  std::chrono::microseconds accumulatedTime_;
};

inline Source::Source(std::string const& iName, unsigned long long iNEvents) :
  file_{TFile::Open(iName.c_str())},
  maxNEvents_{iNEvents},
  accumulatedTime_{std::chrono::microseconds::zero()}
{
  events_ = file_->Get<TTree>("Events");
  auto l = events_->GetListOfBranches();

  dataProducts_.reserve(l->GetEntriesFast());
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
  }
}
#endif
