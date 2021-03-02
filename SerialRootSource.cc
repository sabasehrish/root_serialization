#include "SerialRootSource.h"

#include "TTree.h"
#include "TBranch.h"

#include <iostream>

using namespace cce::tf;

SerialRootSource::SerialRootSource(unsigned iNLanes, unsigned long long iNEvents, std::string const& iName):
  SharedSourceBase(iNEvents),
  file_{TFile::Open(iName.c_str())},
  accumulatedTime_{std::chrono::microseconds::zero()}
 {
  delayedReaders_.reserve(iNLanes);
  dataProductsPerLane_.reserve(iNLanes);
  identifiers_.resize(iNLanes);

  events_ = file_->Get<TTree>("Events");
  nEvents_ = events_->GetEntries();
  auto l = events_->GetListOfBranches();

  const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 

  branches_.reserve(l->GetEntriesFast());
  for(int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    b->SetupAddresses();
    branches_.emplace_back(b);
    if(eventAuxiliaryBranchName == b->GetName()) {
      eventAuxBranch_ = b;
      eventAuxReader_ = EventAuxReader(reinterpret_cast<void**>(b->GetAddress()));
    }
  }
  if(not eventAuxReader_) {
    eventAuxReader_ = EventAuxReader(nullptr);
  }

  for(int laneId=0; laneId < iNLanes; ++laneId) {
    dataProductsPerLane_.emplace_back();
    auto& dataProducts = dataProductsPerLane_.back();
    dataProducts.reserve(branches_.size());
    delayedReaders_.emplace_back(&queue_, &branches_, &dataProducts);
    auto& delayedReader = delayedReaders_.back();

    for(int i=0; i< branches_.size(); ++i) {
      auto b = branches_[i];
      TClass* class_ptr=nullptr;
      EDataType type;
      b->GetExpectedType(class_ptr,type);
      
      dataProducts.emplace_back(i,
                                 reinterpret_cast<void**>(b->GetAddress()),
                                 b->GetName(),
                                 class_ptr,
                                 &delayedReader);
    }
    
  }
}

void SerialRootSource::readEventAsync(unsigned int iLane, long iEventIndex, OptionalTaskHolder iTask) {
  if(nEvents_ > iEventIndex) {
    delayedReaders_[iLane].setEntry(iEventIndex);
    auto temptask = iTask.releaseToTaskHolder();
    auto group = temptask.group();
    queue_.push(*group, [task=std::move(temptask), this, iLane, iEventIndex]() mutable {
        auto start = std::chrono::high_resolution_clock::now();
        if(eventAuxBranch_) {
          eventAuxBranch_->GetEntry(iEventIndex);
          identifiers_[iLane] = eventAuxReader_->doWork();
        }
        accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
        task.doneWaiting();
      });
  }
}

std::chrono::microseconds SerialRootSource::accumulatedTime() const {
  auto fullTime = accumulatedTime_;
  for(auto& delayedReader: delayedReaders_) {
    fullTime += delayedReader.accumulatedTime();
  }
  return fullTime;
}

void SerialRootDelayedRetriever::getAsync(int index, TaskHolder iTask) {
  auto group = iTask.group();
  queue_->push(*group, [index,this, task = std::move(iTask)]() mutable { 
      auto start = std::chrono::high_resolution_clock::now();
      (*dataProducts_)[index].setSize( (*branches_)[index]->GetEntry(entry_) );
      accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
      task.doneWaiting();
    });
};
