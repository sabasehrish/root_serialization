#if !defined(Source_h)
#define Source_h

#include <string>
#include <memory>
#include "TClass.h"
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"

class Source {
public:
  Source(std::string const& iName);

  std::vector<TClass*> const& classForEachBranch() {return classes_;}

  std::vector<TBranch*> const& branches() {return branches_;}

  bool gotoEvent(long iEventIndex) {
    if(iEventIndex<numberOfEvents()) {
      events_->GetEntry(iEventIndex);
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
  std::vector<TBranch*> branches_;
  std::vector<TClass*> classes_;
};

inline Source::Source(std::string const& iName) :
  file_{TFile::Open(iName.c_str())}
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
