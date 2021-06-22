#include "TextDumpOutputer.h"
#include "DataProductRetriever.h"
#include <iostream>

using namespace cce::tf;

void TextDumpOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iProducts) {
  if(iLaneIndex ==0 and summaryDump_) {
    productSizes_ = std::vector<std::atomic<unsigned long long>>(iProducts.size());
    productNames_.reserve(iProducts.size());
    for(auto const& prod: iProducts) {
      productNames_.emplace_back(prod.name());
    }
  }
}


void TextDumpOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iProduct, TaskHolder iCallback) const {
  if(summaryDump_) {
    auto& s = productSizes_[iProduct.index()];
    s += iProduct.size();
  }
  if(perEventDump_) {
    queue_.push(*iCallback.group(), [callback = std::move(iCallback), iLaneIndex, &iProduct]() mutable {
        std::cout <<"lane: "<<iLaneIndex<<" product: "<<iProduct.name()<<" size:"<<iProduct.size()<<"\n";
        callback.doneWaiting();
      });
  }
}

void TextDumpOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  if(perEventDump_) {
    queue_.push(*iCallback.group(), [callback = std::move(iCallback), iLaneIndex, iEventID]() mutable {
        std::cout <<"lane: "<<iLaneIndex<<" finished event:"<<iEventID.run<<" "<<iEventID.lumi<<" "<<iEventID.event<<"\n";
        callback.doneWaiting();
      });
  }
  if(summaryDump_) {
    ++eventCount_;
  }
}


void TextDumpOutputer::printSummary() const {
  if(summaryDump_) {
    auto itSize = productSizes_.begin();
    auto nEvents = eventCount_.load();
    for(auto const& name: productNames_) {
      double aveSize = double(itSize->load())/nEvents;
      std::cout <<"product: "<<name<<" ave size: "<<aveSize<<"\n";
      ++itSize;
    }
  }
}

