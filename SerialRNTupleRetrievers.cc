#include "SerialRNTupleRetrievers.h"

using namespace cce::tf;

void SerialRNTuplePromptRetriever::getAsync(DataProductRetriever& dataProduct, int index, TaskHolder iTask) {
  auto start = std::chrono::high_resolution_clock::now();
  dataProduct.setSize(0);
  accumulatedTime_ += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
  iTask.doneWaiting();
};

void SerialRNTupleDelayedRetriever::fillViews(ROOT::Experimental::RNTupleReader& iReader, std::vector<std::string> const& iFieldIDs) {
  views_.reserve(iFieldIDs.size());
  for(auto const& id: iFieldIDs) {
    views_.push_back(iReader.GetView<void>(id));
  }
}

void SerialRNTupleDelayedRetriever::getAsync(DataProductRetriever& dataProduct, int index, TaskHolder iTask) {
  auto group = iTask.group();
  queue_->push(*group, [&dataProduct, index,this, task = std::move(iTask)]() mutable { 
      auto start = std::chrono::high_resolution_clock::now();
      views_[index](eventIndex_);
      (*addresses_)[index] = views_[index].GetValue().GetPtr<void>().get();
      dataProduct.setSize(0);
      accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
      task.doneWaiting();
    });
};
