#include "SerialRNTupleSource.h"
#include "SourceFactory.h"

#include "TTree.h"
#include "TBranch.h"

#include <iostream>

using namespace cce::tf;

SerialRNTupleSource::SerialRNTupleSource(unsigned iNLanes, unsigned long long iNEvents, std::string const& iName, bool iDelayReading):
  SharedSourceBase(iNEvents),
  events_{ROOT::Experimental::RNTupleReader::Open("Events", iName.c_str())},
  accumulatedTime_{std::chrono::microseconds::zero()},
  delayReading_{iDelayReading}
 {
  if(not delayReading_) {
     promptReaders_.reserve(iNLanes);
  } else {
    delayedReaders_.reserve(iNLanes);
  }
  dataProductsPerLane_.reserve(iNLanes);
  ptrToDataProducts_.resize(iNLanes);
  entries_.reserve(iNLanes);
  identifiers_.resize(iNLanes);

  nEvents_ = events_->GetNEntries();
  if (iNEvents < nEvents_ ) nEvents_ = iNEvents;
  
  const std::string eventIDBranchName{"EventID"}; 

  bool hasEventID = false;
  bool hasEventAux = false;
  auto const& model = events_->GetModel();
  auto const& subfields = model.GetFieldZero().GetSubFields();
  fieldIDs_.reserve(subfields.size());
  std::vector<std::string> fieldType;
  fieldType.reserve(subfields.size());
  for(auto* field: subfields) {
    if(eventIDBranchName == field->GetFieldName()) {
      hasEventID = true;
      continue;
    }
    fieldIDs_.emplace_back(field->GetFieldName());
    fieldType.emplace_back(field->GetTypeName());
  }

  for(int laneId=0; laneId < iNLanes; ++laneId) {
    entries_.emplace_back(model.CreateEntry());
    dataProductsPerLane_.emplace_back();
    auto& dataProducts = dataProductsPerLane_.back();
    
    dataProducts.reserve(fieldIDs_.size());
    DelayedProductRetriever* delayedReader = nullptr;
    if(not delayReading_) {
      promptReaders_.emplace_back(&queue_, entries_.back().get(),&fieldIDs_);
      delayedReader = &promptReaders_.back();
    } else {
      delayedReaders_.emplace_back(&queue_, *events_.get(), fieldIDs_, &ptrToDataProducts_[laneId]);
      delayedReader = &delayedReaders_.back();
    }

    auto& addressDataProducts = ptrToDataProducts_[laneId];
    addressDataProducts.reserve(fieldIDs_.size());
    for(int i=0; i< fieldIDs_.size(); ++i) {
      TClass* class_ptr=TClass::GetClass(fieldType[i].c_str());
      addressDataProducts.push_back(entries_.back()->GetPtr<void>(fieldIDs_[i]).get());
      dataProducts.emplace_back(i,
                                &addressDataProducts[i],
                                fieldIDs_[i],
                                class_ptr,
                                delayedReader);
    }
  }
}

void SerialRNTupleSource::readEventAsync(unsigned int iLane, long iEventIndex, OptionalTaskHolder iTask) {
  if(nEvents_ > iEventIndex) {
    auto temptask = iTask.releaseToTaskHolder();
    auto group = temptask.group();
    queue_.push(*group, [task=std::move(temptask), this, iLane, iEventIndex]() mutable {
        auto start = std::chrono::high_resolution_clock::now();
        if (delayReading_) {
          identifiers_[iLane] = events_->GetView<cce::tf::EventIdentifier>("EventID")(iEventIndex);
          delayedReaders_[iLane].setEventIndex(iEventIndex);
        } else {
          events_->LoadEntry(iEventIndex, *entries_[iLane]);
          identifiers_[iLane] = *entries_[iLane]->GetPtr<cce::tf::EventIdentifier>("EventID");
        }
        accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
        task.doneWaiting();
      });
  }
}

std::chrono::microseconds SerialRNTupleSource::accumulatedTime() const {
  auto fullTime = accumulatedTime_;
  for(auto& delayedReader: promptReaders_) {
    fullTime += delayedReader.accumulatedTime();
  }
  return fullTime;
}

void SerialRNTupleSource::printSummary() const {
  std::chrono::microseconds sourceTime = accumulatedTime();
  std::cout <<"\nSource time: "<<sourceTime.count()<<"us\n"<<std::endl;
}

void SerialRNTuplePromptRetriever::getAsync(DataProductRetriever& dataProduct, int index, TaskHolder iTask) {
  auto group = iTask.group();
  queue_->push(*group, [&dataProduct, index,this, task = std::move(iTask)]() mutable { 
      auto start = std::chrono::high_resolution_clock::now();
      dataProduct.setSize(0);
      accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
      task.doneWaiting();
    });
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

namespace {
    class Maker : public SourceMakerBase {
  public:
    Maker(): SourceMakerBase("SerialRNTupleSource") {}
      std::unique_ptr<SharedSourceBase> create(unsigned int iNLanes, unsigned long long iNEvents, ConfigurationParameters const& params) const final {
        auto fileName = params.get<std::string>("fileName");
        if(not fileName) {
          std::cout <<"no file name given\n";
          return {};
        }
        return std::make_unique<SerialRNTupleSource>(iNLanes, iNEvents, *fileName, params.get<bool>("delayReading",false));
    }
    };

  Maker s_maker;
}
