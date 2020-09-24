#include "RepeatingRootSource.h"
#include <iostream>
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"

using namespace cce::tf;

RepeatingRootSource::RepeatingRootSource(std::string const& iName, unsigned int iNUniqueEvents) :
  SourceBase(),
  nUniqueEvents_(iNUniqueEvents),
  identifierPerEvent_(iNUniqueEvents),
  dataBuffersPerEvent_(iNUniqueEvents)
{
  auto file_ = std::unique_ptr<TFile>(TFile::Open(iName.c_str()));
  auto events = file_->Get<TTree>("Events");
  auto l = events->GetListOfBranches();

  const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
  int eventAuxIndex = -1;
  dataProducts_.reserve(l->GetEntriesFast());
  std::vector<TBranch*> branches;
  branches.reserve(l->GetEntriesFast());
  for( int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    b->SetupAddresses();
    TClass* class_ptr=nullptr;
    EDataType type;
    b->GetExpectedType(class_ptr,type);
    assert(class_ptr != nullptr);
    dataProducts_.emplace_back(i,
			       nullptr,
                               b->GetName(),
                               class_ptr,
			       &delayedReader_);
    branches.emplace_back(b);
    if(eventAuxiliaryBranchName == dataProducts_.back().name()) {
      eventAuxIndex = i;
    }
  }

  for(int i=0; i<nUniqueEvents_; ++i) {
    fillBuffer(i, dataBuffersPerEvent_[i], branches);
    if(eventAuxIndex != -1) {
      identifierPerEvent_[i] = EventAuxReader(&dataBuffersPerEvent_[i][eventAuxIndex].address_).doWork();
      //std::cout <<"id "<<identifierPerEvent_[i].event<<std::endl;
    } else {
      identifierPerEvent_[i] = {1,1,static_cast<unsigned long long>(i)};
    }
  }  
}

RepeatingRootSource::~RepeatingRootSource() {
  for(auto& buffers: dataBuffersPerEvent_) {
    auto it = buffers.begin();
    for(auto& d: dataProducts_) {
      d.classType()->Destructor(it->address_);
      ++it;
    }
  }
}


bool RepeatingRootSource::readEvent(long iEventIndex) {
  presentEventIndex_ = iEventIndex % nUniqueEvents_;
  auto it = dataBuffersPerEvent_[presentEventIndex_].begin();
  for(auto& d: dataProducts_) {
    d.setAddress(&it->address_);
    d.setSize(it->size_);
    ++it;
  }
  return true;
}

void RepeatingRootSource::fillBuffer(int iEntry, std::vector<BufferInfo>& bi, std::vector<TBranch*>& branches) {
  bi.reserve(branches.size());

  for(int i=0; i< branches.size(); ++i) {
    void* object = dataProducts_[i].classType()->New();
    auto branch = branches[i];
    branch->SetAddress(&object);
    auto s = branch->GetEntry(iEntry);
    branch->SetAddress(nullptr);
    bi.emplace_back( object, static_cast<size_t>(s));
  }
}
