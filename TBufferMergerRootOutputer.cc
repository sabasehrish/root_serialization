
#include <iostream>

#include "TBufferMergerRootOutputer.h"

#include "TTree.h"
#include "TBranch.h"
#include "TROOT.h"

#include "tbb/task_arena.h"
#include "tbb/task_group.h"

using namespace cce::tf;

namespace {
  ROOT::ECompressionAlgorithm algorithmChoice(std::string const& iName) {
    if(not iName.empty()) {
      if(iName == "ZLIB") {
        return ROOT::kZLIB;
      } else if(iName == "LZMA") {
        return ROOT::kLZMA;
      } else if(iName == "LZ4") {
        return ROOT::kLZ4;
      } else {
        std::cout <<"unknown compression algorithm "<<iName<<std::endl;
        abort();
      }
    }
    return ROOT::kUseGlobalSetting;
  }
}

TBufferMergerRootOutputer::TBufferMergerRootOutputer(std::string const& iFileName, unsigned int iNLanes, Config const& iConfig): 
  buffer_(iFileName.c_str(), "recreate", ROOT::CompressionSettings(algorithmChoice(iConfig.compressionAlgorithm_),iConfig.compressionLevel_)),
  lanes_{std::size_t(iNLanes)},
                    basketSize_{iConfig.basketSize_},
                    splitLevel_{iConfig.splitLevel_},
                    treeMaxVirtualSize_{iConfig.treeMaxVirtualSize_},
                    autoFlush_{iConfig.autoFlush_ != -1 ? iConfig.autoFlush_ : Config::kDefaultAutoFlush }
{

}

TBufferMergerRootOutputer::~TBufferMergerRootOutputer() {
}

void TBufferMergerRootOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  auto& lane = lanes_[iLaneIndex];
  lane.file_ = buffer_.GetFile();
  lane.eventTree_ = new TTree("Events","", splitLevel_, lane.file_.get());
  //Turn off auto save
  lane.eventTree_->SetAutoSave(std::numeric_limits<Long64_t>::max());

  if (treeMaxVirtualSize_ >= 0) {
    lane.eventTree_->SetMaxVirtualSize(static_cast<Long64_t>(treeMaxVirtualSize_));
  }
  lane.retrievers_ = &iDPs;
  lane.branches_.reserve(iDPs.size());
  for(auto& dp : iDPs) {
    lane.branches_.push_back( lane.eventTree_->Branch(dp.name().c_str(), dp.classType()->GetName(), dp.address(), basketSize_) );
  }
  lane.accumulatedTime_ = std::chrono::microseconds::zero();

}

void TBufferMergerRootOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
}

void TBufferMergerRootOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  
  auto group = iCallback.group();

  group->run([this, iLaneIndex, callback=std::move(iCallback)]()  {
      const_cast<TBufferMergerRootOutputer*>(this)->write(iLaneIndex);
      const_cast<cce::tf::TaskHolder&>(callback).doneWaiting();
    });
}

void TBufferMergerRootOutputer::write(unsigned int iLaneIndex) {

  auto start = std::chrono::high_resolution_clock::now();

  auto& lane = lanes_[iLaneIndex];
  auto* retrievers = lane.retrievers_;

  auto it = lane.branches_.begin();
  for(auto const& retriever: *retrievers) {
    (*it)->SetAddress(retriever.address());
    ++it;
  }

  // Isolate the fill operation so that IMT doesn't grab other large tasks
  // that could lead to stalling
  tbb::this_task_arena::isolate([&] { 
      assert(lane.eventTree_); 
      lane.nBytesWrittenSinceLastWrite_ +=lane.eventTree_->Fill();
      ++lane.nEventsSinceWrite_;
      if(autoFlush_ <0) {
	//Flush based on number of bytes written to this buffer
	if(lane.nBytesWrittenSinceLastWrite_ > -1*autoFlush_) {
	  std::cout <<"lane "<< iLaneIndex<<" events since write "<<lane.nEventsSinceWrite_<<" "<<lane.nBytesWrittenSinceLastWrite_ << std::endl;
	  lane.nBytesWrittenSinceLastWrite_ = 0;
	  lane.nEventsSinceWrite_ = 0;
	  lane.file_->Write();
	}
      } else {
	//Flush based on total number of events filled across all  buffers
	auto v = ++numberEventsSinceLastWrite_;
	if(v == autoFlush_) {
	  //NOTE: we do not care if this number was incremented
	  // after the check on another thread since we will write regardless.
	  numberEventsSinceLastWrite_ = 0;
	  for(auto& lane: lanes_){
	    lane.shouldWrite_ = true;
	  }
	}
	if(lane.shouldWrite_) {
	  lane.shouldWrite_=false;
	  lane.file_->Write();
	}
      }
    });

  lane.accumulatedTime_ += std::chrono::duration_cast<decltype(lane.accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
}
  
void TBufferMergerRootOutputer::printSummary() const {

  std::cout <<"end write"<<std::endl;
  auto start = std::chrono::high_resolution_clock::now();

  tbb::task_group group;
  for(auto& lane: lanes_) {
    group.run([&lane]() {
	std::cout <<" start lane"<<std::endl;
	lane.file_->Write();
	std::cout <<" finish lane"<<std::endl;
      });
  }
  group.wait();
  auto writeTime = std::chrono::duration_cast<decltype(lanes_[0].accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);

  std::cout <<"file close"<<std::endl;
  start = std::chrono::high_resolution_clock::now();
  for(auto& lane: lanes_) {
    std::cout <<" start next lane"<<std::endl;
    lane.file_->Close();
  }
  auto closeTime = std::chrono::duration_cast<decltype(lanes_[0].accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);

  decltype(lanes_[0].accumulatedTime_.count()) sum = 0;
  for(auto& l: lanes_) {
    sum += l.accumulatedTime_.count();
  }
  

  std::cout <<"TBufferMergerRootOutputer loop time: "<<sum<<"us\n";
  std::cout <<"TBufferMergerRootOutputer end write time: "<<writeTime.count()<<"us\n";
  std::cout <<"TBufferMergerRootOutputer end close time: "<<closeTime.count()<<"us\n";
  std::cout <<"TBufferMergerRootOutputer total time: "<<sum+writeTime.count()<<"us\n";

}
