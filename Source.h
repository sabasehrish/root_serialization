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

class Source {
public:
  Source(std::string const& iName);
  Source(Source&&) = default;
  Source(Source const&) = default;

  std::vector<TClass*> const& classForEachBranch() {return classes_;}

  std::vector<TBranch*> const& branches() {return branches_;}

  bool gotoEvent(long iEventIndex) {
    if(iEventIndex<numberOfEvents()) {
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
  TTree* events_;
  std::vector<TBranch*> branches_;
  std::vector<TClass*> classes_;
  std::chrono::microseconds accumulatedTime_;
};

inline Source::Source(std::string const& iName) :
  file_{TFile::Open(iName.c_str())},
  accumulatedTime_{std::chrono::microseconds::zero()}
{
  events_ = file_->Get<TTree>("Events");
  auto l = events_->GetListOfBranches();

  classes_.reserve(l->GetEntriesFast());
  branches_.reserve(l->GetEntriesFast());
  for( int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    branches_.push_back(b);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    b->SetupAddresses();
    TClass* class_ptr=nullptr;
    EDataType type;
    b->GetExpectedType(class_ptr,type);
    classes_.push_back( class_ptr);
  }
}
#endif
