#include "SerialRootTreeGetEntrySource.h"
#include "SourceFactory.h"

#include "TTree.h"
#include "TBranch.h"

#include <iostream>

using namespace cce::tf;

SerialRootTreeGetEntrySource::SerialRootTreeGetEntrySource(unsigned iNLanes, unsigned long long iNEvents, std::string const& iName):
  SharedSourceBase(iNEvents),
  file_{TFile::Open(iName.c_str())},
  eventAuxReader_{*file_},
  accumulatedTime_{std::chrono::microseconds::zero()}
 {
  delayedReaders_.reserve(iNLanes);
  dataProductsPerLane_.reserve(iNLanes);
  identifiers_.resize(iNLanes);

  events_ = file_->Get<TTree>("Events");
  nEvents_ = events_->GetEntries();
  auto l = events_->GetListOfBranches();

  const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
  const std::string eventIDBranchName{"EventID"}; 

  branches_.reserve(l->GetEntriesFast());
  for(int i=0; i< l->GetEntriesFast(); ++i) {
    auto b = dynamic_cast<TBranch*>((*l)[i]);
    //std::cout<<b->GetName()<<std::endl;
    //std::cout<<b->GetClassName()<<std::endl;
    if(eventIDBranchName == b->GetName()) {
      eventIDBranch_ = b;
      continue;
    }
    b->SetupAddresses();
    branches_.emplace_back(b);
    if(eventAuxiliaryBranchName == b->GetName()) {
      eventAuxBranch_ = b;
    }
  }

  for(int laneId=0; laneId < iNLanes; ++laneId) {
    dataProductsPerLane_.emplace_back();
    auto& dataProducts = dataProductsPerLane_.back();
    dataProducts.reserve(branches_.size());
    delayedReaders_.emplace_back(branches_);
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

void SerialRootTreeGetEntrySource::readEventAsync(unsigned int iLane, long iEventIndex, OptionalTaskHolder iTask) {
  if(nEvents_ > iEventIndex) {
    auto temptask = iTask.releaseToTaskHolder();
    auto group = temptask.group();
    queue_.push(*group, [task=std::move(temptask), this, iLane, iEventIndex]() mutable {
        auto start = std::chrono::high_resolution_clock::now();
        delayedReaders_[iLane].setAddresses(branches_);
        events_->GetEntry(iEventIndex);
        if(eventAuxBranch_) {
          eventAuxBranch_->GetEntry(iEventIndex);
          identifiers_[iLane] = eventAuxReader_.doWork(eventAuxBranch_);
        } else if(eventIDBranch_) {
          eventIDBranch_->SetAddress(&identifiers_[iLane]);
          eventIDBranch_->GetEntry(iEventIndex);
        }
        accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
        task.doneWaiting();
      });
  }
}

std::chrono::microseconds SerialRootTreeGetEntrySource::accumulatedTime() const {
  auto fullTime = accumulatedTime_;
  return fullTime;
}

void SerialRootTreeGetEntrySource::printSummary() const {
  std::chrono::microseconds sourceTime = accumulatedTime();
  std::cout <<"\nSource time: "<<sourceTime.count()<<"us\n"<<std::endl;
}

void SerialRootTreeGetEntryDelayedRetriever::setupBuffer(std::vector<TBranch*> const& iBranches) {
  buffers_.reserve(iBranches.size());
  for(auto b : iBranches) {
    b->SetupAddresses();
    TClass* class_ptr=nullptr;
    EDataType type;
    b->GetExpectedType(class_ptr,type);
    assert(class_ptr != nullptr);
    buffers_.push_back(class_ptr->New());
  }
}

void SerialRootTreeGetEntryDelayedRetriever::setAddresses(std::vector<TBranch*> const& iBranches) {
  for(int index = 0; auto branch: iBranches) {
    branch->SetAddress(&buffers_[index]);
    ++index;
  }
}

void SerialRootTreeGetEntryDelayedRetriever::getAsync(DataProductRetriever& dataProduct, int index, TaskHolder iTask) {
  dataProduct.setSize(0);
  dataProduct.setAddress(&buffers_[index]);
}

namespace {
    class Maker : public SourceMakerBase {
  public:
    Maker(): SourceMakerBase("SerialRootTreeGetEntrySource") {}
      std::unique_ptr<SharedSourceBase> create(unsigned int iNLanes, unsigned long long iNEvents, ConfigurationParameters const& params) const final {
        auto fileName = params.get<std::string>("fileName");
        if(not fileName) {
          std::cout <<"no file name given\n";
          return {};
        }
        return std::make_unique<SerialRootTreeGetEntrySource>(iNLanes, iNEvents, *fileName);
    }
    };

  Maker s_maker;
}
