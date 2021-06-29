#include "RepeatingRootSource.h"
#include <iostream>
#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"
#include <unordered_set>

using namespace cce::tf;

RepeatingRootSource::RepeatingRootSource(std::string const& iName, unsigned int iNUniqueEvents, unsigned int iNLanes, unsigned long long iNEvents, std::string const& iBranchToRead) :
  SharedSourceBase(iNEvents),
  nUniqueEvents_(iNUniqueEvents),
  dataProductsPerLane_(iNLanes),
  identifierPerEvent_(iNUniqueEvents),
  dataBuffersPerEvent_(iNUniqueEvents),
  accumulatedTime_(0)
{
  auto file_ = std::unique_ptr<TFile>(TFile::Open(iName.c_str()));
  auto events = file_->Get<TTree>("Events");
  auto l = events->GetListOfBranches();

  const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
  const std::string eventIDBranchName{"EventID"}; 
  TBranch* eventIDBranch = nullptr;
  int eventAuxIndex = -1;
  for(auto& dataProducts: dataProductsPerLane_) {
    dataProducts.reserve(l->GetEntriesFast());
  }
  std::unordered_set<std::string> branchesToRead;
  if(not iBranchToRead.empty()) {
    branchesToRead.insert(iBranchToRead);
    branchesToRead.insert(eventAuxiliaryBranchName);
  }
  std::vector<TBranch*> branches;
  branches.reserve(l->GetEntriesFast());
  int index=0;
  for( int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    if(eventIDBranchName == b->GetName()) {
      eventIDBranch = b;
      continue;
    }
    b->SetupAddresses();
    TClass* class_ptr=nullptr;
    EDataType type;
    b->GetExpectedType(class_ptr,type);
    assert(class_ptr != nullptr);
    if((not branchesToRead.empty()) and branchesToRead.end() == branchesToRead.find(b->GetName())) {
      continue;
    }
    std::cout<<b->GetName()<<std::endl;
    for(auto& dataProducts: dataProductsPerLane_) {
      dataProducts.emplace_back(index,
                                nullptr,
                                b->GetName(),
                                class_ptr,
                                &delayedReader_);
    }
    branches.emplace_back(b);
    if(eventAuxiliaryBranchName == dataProductsPerLane_[0].back().name()) {
      eventAuxIndex = index;
    }
    ++index;
  }

  for(int i=0; i<nUniqueEvents_; ++i) {
    fillBuffer(i, dataBuffersPerEvent_[i], branches);
    if(eventAuxIndex != -1) {
      auto addr = &dataBuffersPerEvent_[i][eventAuxIndex].address_;
      EventAuxReader aux_reader;
      if (file_->GetKey("RootFileDB")) {
        // art/ROOT file
        aux_reader = EventAuxReader([addr]{ return artEventID(addr); });
      } else {
        // CMSSW file
        aux_reader = EventAuxReader([addr]{ return cmsEventID(addr); });
      }
      identifierPerEvent_[i] = aux_reader.doWork();
      //std::cout <<"id "<<identifierPerEvent_[i].event<<std::endl;
    } else if(eventIDBranch) {
      eventIDBranch->SetAddress(&identifierPerEvent_[i]);
      eventIDBranch->GetEntry(i);
    } else {
      identifierPerEvent_[i] = {1,1,static_cast<unsigned long long>(i)};
    }
  }  
}

RepeatingRootSource::~RepeatingRootSource() {
  for(auto& dataProducts: dataProductsPerLane_) {
    for(auto& d: dataProducts) {
      d.setAddress(nullptr);
    }
  }
  for(auto& buffers: dataBuffersPerEvent_) {
    auto it = buffers.begin();
    for(auto& d: dataProductsPerLane_[0]) {
      d.classType()->Destructor(it->address_);
      ++it;
    }
  }
}


void RepeatingRootSource::readEventAsync(unsigned int iLane, long iEventIndex, OptionalTaskHolder iTask) {
  auto start = std::chrono::high_resolution_clock::now();
  auto presentEventIndex = iEventIndex % nUniqueEvents_;
  auto it = dataBuffersPerEvent_[presentEventIndex].begin();
  auto& dataProducts = dataProductsPerLane_[iLane];
  for(auto& d: dataProducts) {
    d.setAddress(&it->address_);
    d.setSize(it->size_);
    ++it;
  }
  auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  accumulatedTime_ += time.count();
  iTask.runNow();
}

void RepeatingRootSource::fillBuffer(int iEntry, std::vector<BufferInfo>& bi, std::vector<TBranch*>& branches) {
  bi.reserve(branches.size());

  for(int i=0; i< branches.size(); ++i) {
    void* object = dataProductsPerLane_[0][i].classType()->New();
    auto branch = branches[i];
    branch->SetAddress(&object);
    auto s = branch->GetEntry(iEntry);
    branch->SetAddress(nullptr);
    bi.emplace_back( object, static_cast<size_t>(s));
  }
}

void RepeatingRootSource::printSummary() const {
      std::chrono::microseconds sourceTime = accumulatedTime();
      std::cout <<"\nSource time: "<<sourceTime.count()<<"us\n"<<std::endl;
}
