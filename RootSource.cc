#include "RootSource.h"
#include <iostream>
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"

using namespace cce::tf;

RootSource::RootSource(std::string const& iName) :
  file_{TFile::Open(iName.c_str())}
{
  events_ = file_->Get<TTree>("Events");
  auto l = events_->GetListOfBranches();

  const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
  const std::string eventIDBranchName{"EventID"}; 

  dataProducts_.reserve(l->GetEntriesFast());
  branches_.reserve(l->GetEntriesFast());

  void** aux_branch{nullptr};
  for( int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    if(eventIDBranchName == b->GetName()) {
      eventIDBranch_ = b;
      continue;
    }
    b->SetupAddresses();
    TClass* class_ptr=nullptr;
    EDataType type;
    b->GetExpectedType(class_ptr,type);

    dataProducts_.emplace_back(i,
			       reinterpret_cast<void**>(b->GetAddress()),
                               b->GetName(),
                               class_ptr,
			       &delayedReader_);
    branches_.emplace_back(b);
    if(eventAuxiliaryBranchName == dataProducts_.back().name()) {
      aux_branch = dataProducts_.back().address();
    }
  }
  if(aux_branch) {
    eventAuxReader_.bindToBranch(aux_branch);
  }
}

EventIdentifier RootSource::eventIdentifier() {
  if(eventIDBranch_) {
    return id_;
  }
  return eventAuxReader_.doWork();
}

long RootSource::numberOfEvents() { 
  return events_->GetEntriesFast();
}

bool RootSource::readEvent(long iEventIndex) {
  if(iEventIndex<numberOfEvents()) {
    if(eventIDBranch_) {
      eventIDBranch_->SetAddress(&id_);
      eventIDBranch_->GetEntry(iEventIndex);
    }

    auto it = dataProducts_.begin();
    for(auto b: branches_) {
      (it++)->setSize( b->GetEntry(iEventIndex) );
    }
    return true;
  }
  return false;
}
