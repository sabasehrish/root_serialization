
#include <iostream>

#include "RootOutputer.h"

#include "TTree.h"
#include "TBranch.h"
#include "TROOT.h"

#include "tbb/task_arena.h"

using namespace cce::tf;

RootOutputer::RootOutputer(std::string const& iFileName, unsigned int iNLanes, Config const& iConfig): 
  file_(iFileName.c_str(), "recreate", "", iConfig.compressionLevel_),
  eventTree_(new TTree("Events","", iConfig.splitLevel_, &file_)),
  retrievers_{std::size_t(iNLanes)},
  accumulatedTime_(std::chrono::microseconds::zero()),
  basketSize_{iConfig.basketSize_}
{
  if(not iConfig.compressionAlgorithm_.empty()) {
    if(iConfig.compressionAlgorithm_ == "ZLIB") {
      file_.SetCompressionAlgorithm(ROOT::kZLIB);
    } else if(iConfig.compressionAlgorithm_ == "LZMA") {
      file_.SetCompressionAlgorithm(ROOT::kLZMA);
    } else if(iConfig.compressionAlgorithm_ == "LZ4") {
      file_.SetCompressionAlgorithm(ROOT::kLZ4);
    } else {
      std::cout <<"unknown compression algorithm "<<iConfig.compressionAlgorithm_<<std::endl;
      abort();
    }
  }

  //Turn off auto save
  eventTree_->SetAutoSave(std::numeric_limits<Long64_t>::max());

  if (iConfig.treeMaxVirtualSize_ >= 0) {
    eventTree_->SetMaxVirtualSize(static_cast<Long64_t>(iConfig.treeMaxVirtualSize_));
  }

}

RootOutputer::~RootOutputer() {
  file_.Write();
  file_.Close();
}

void RootOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  retrievers_[iLaneIndex] = &iDPs;
  if(iLaneIndex == 0) {
    branches_.reserve(iDPs.size());
    for(auto& dp : iDPs) {
      branches_.push_back( eventTree_->Branch(dp.name().c_str(), dp.classType()->GetName(), dp.address(), basketSize_) );
    }
  }
}

void RootOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
}

void RootOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto group = iCallback.group();
  queue_.push(*group, [this, iLaneIndex, callback=std::move(iCallback)]() mutable {
      const_cast<RootOutputer*>(this)->write(iLaneIndex);
      callback.doneWaiting();
    });
}

void RootOutputer::write(unsigned int iLaneIndex) {

  auto start = std::chrono::high_resolution_clock::now();

  auto* retrievers = retrievers_[iLaneIndex];

  auto it = branches_.begin();
  for(auto const& retriever: *retrievers) {
    (*it)->SetAddress(retriever.address());
    ++it;
  }

  // Isolate the fill operation so that IMT doesn't grab other large tasks
  // that could lead to stalling
  tbb::this_task_arena::isolate([&] { eventTree_->Fill(); });

  accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
}
  
void RootOutputer::printSummary() const {
  std::cout <<"RootOutputer total time: "<<accumulatedTime_.count()<<"us\n";
}
