#include <iostream>

#include "RootOutputer.h"
#include "RootOutputerConfig.h"
#include "OutputerFactory.h"

#include "TTree.h"
#include "TBranch.h"
#include "TROOT.h"
#include "TFileCacheWrite.h"

#include "tbb/task_arena.h"

using namespace cce::tf;

RootOutputer::RootOutputer(std::string const& iFileName, unsigned int iNLanes, Config const& iConfig): 
  file_(iFileName.c_str(), "recreate", "", iConfig.compressionLevel_),
  eventTree_(new TTree("Events","", iConfig.splitLevel_, &file_)),
  retrievers_{std::size_t(iNLanes)},
  accumulatedTime_(std::chrono::microseconds::zero()),
  basketSize_{iConfig.basketSize_},
  splitLevel_{iConfig.splitLevel_}
{
  TFileCacheWrite(&file_, iConfig.cacheSize_);
  if(not iConfig.compressionAlgorithm_.empty()) {
    if(iConfig.compressionAlgorithm_ == "ZLIB") {
      file_.SetCompressionAlgorithm(ROOT::kZLIB);
    } else if(iConfig.compressionAlgorithm_ == "LZMA") {
      file_.SetCompressionAlgorithm(ROOT::kLZMA);
    } else if(iConfig.compressionAlgorithm_ == "LZ4") {
      file_.SetCompressionAlgorithm(ROOT::kLZ4);
    } else if(iConfig.compressionAlgorithm_ == "ZSTD") {
      file_.SetCompressionAlgorithm(ROOT::kZSTD);
    }else {
      std::cout <<"unknown compression algorithm "<<iConfig.compressionAlgorithm_<<std::endl;
      abort();
    }
  }

  //Turn off auto save
  eventTree_->SetAutoSave(std::numeric_limits<Long64_t>::max());
  if(-1 != iConfig.autoFlush_) {
    eventTree_->SetAutoFlush(iConfig.autoFlush_);
  }

  if (iConfig.treeMaxVirtualSize_ >= 0) {
    eventTree_->SetMaxVirtualSize(static_cast<Long64_t>(iConfig.treeMaxVirtualSize_));
  }
}

RootOutputer::~RootOutputer() {
}

void RootOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
  retrievers_[iLaneIndex] = &iDPs;
  if(iLaneIndex == 0) {
    bool hasEventAuxiliaryBranch = false;
    branches_.reserve(iDPs.size());
    for(auto& dp : iDPs) {
      branches_.push_back( eventTree_->Branch(dp.name().c_str(), dp.classType()->GetName(), dp.address(), basketSize_, splitLevel_) );
      if(dp.name() == eventAuxiliaryBranchName) {
        hasEventAuxiliaryBranch = true;
      }
    }
    if(not hasEventAuxiliaryBranch) {
      eventIDBranch_ = eventTree_->Branch("EventID", &id_, "run/i:lumi/i:event/l");
    }
  }
}

void RootOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
}

void RootOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto group = iCallback.group();
  queue_.push(*group, [this, iLaneIndex, callback=std::move(iCallback), iEventID]() mutable {
      const_cast<RootOutputer*>(this)->write(iLaneIndex, iEventID);
      callback.doneWaiting();
    });
}

void RootOutputer::write(unsigned int iLaneIndex, EventIdentifier const& iEventID) {

  auto start = std::chrono::high_resolution_clock::now();

  auto* retrievers = retrievers_[iLaneIndex];

  auto it = branches_.begin();
  for(auto const& retriever: *retrievers) {
    (*it)->SetAddress(retriever.address());
    ++it;
  }
  id_ = iEventID;

  // Isolate the fill operation so that IMT doesn't grab other large tasks
  // that could lead to stalling
  tbb::this_task_arena::isolate([&] { eventTree_->Fill(); });

  accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
}
  
void RootOutputer::printSummary() const {
  auto start = std::chrono::high_resolution_clock::now();
  file_.Write();
  auto writeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

  start = std::chrono::high_resolution_clock::now();
  file_.Close();
  auto closeTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

  std::cout <<"RootOutputer total time: "<<accumulatedTime_.count()<<"us\n";
  std::cout << "  end of job file write time: "<<writeTime.count()<<"us\n";
  std::cout << "  end of job file close time: "<<closeTime.count()<<"us\n";
}

namespace {
  class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("RootOutputer") {}
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {
      auto result = parseRootConfig(params);
      if(not result) {
        return {};
      }
      return std::make_unique<RootOutputer>(result->first,iNLanes, outputerConfig<RootOutputer::Config>(result->second));
    }
    };

  Maker s_maker;
}
