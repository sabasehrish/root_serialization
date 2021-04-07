#include "TextDumpOutputer.h"
#include "DataProductRetriever.h"
#include <iostream>

using namespace cce::tf;

void TextDumpOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iProduct, TaskHolder iCallback) const {
  queue_.push(*iCallback.group(), [callback = std::move(iCallback), iLaneIndex, &iProduct]() mutable {
      std::cout <<"lane: "<<iLaneIndex<<" product: "<<iProduct.name()<<" size:"<<iProduct.size()<<"\n";
      callback.doneWaiting();
    });
}

void TextDumpOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  queue_.push(*iCallback.group(), [callback = std::move(iCallback), iLaneIndex, iEventID]() mutable {
      std::cout <<"lane: "<<iLaneIndex<<" finished event:"<<iEventID.run<<" "<<iEventID.lumi<<" "<<iEventID.event<<"\n";
      callback.doneWaiting();
    });
}


