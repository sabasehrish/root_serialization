#include "SerialRNTupleSource.h"
#include "SourceFactory.h"

#include "TTree.h"
#include "TBranch.h"

#include <iostream>

using namespace cce::tf;

SerialRNTupleSource::SerialRNTupleSource(unsigned iNLanes, unsigned long long iNEvents, std::string const& iName):
  SharedSourceBase(iNEvents),
  events_{ROOT::Experimental::RNTupleReader::Open("Events", iName.c_str())},
  accumulatedTime_{std::chrono::microseconds::zero()}
 {
  delayedReaders_.reserve(iNLanes);
  dataProductsPerLane_.reserve(iNLanes);
  ptrToDataProducts_.resize(iNLanes);
  entries_.reserve(iNLanes);
  identifiers_.resize(iNLanes);

  nEvents_ = events_->GetNEntries();
  if (iNEvents < nEvents_ ) nEvents_ = iNEvents;
  
  const std::string eventIDBranchName{"EventID"}; 

  bool hasEventID = false;
  bool hasEventAux = false;
  auto model = events_->GetModel();
  auto const& subfields = model->GetFieldZero().GetSubFields();
  fieldIDs_.reserve(subfields.size());
  std::vector<std::string> fieldType;
  fieldType.reserve(subfields.size());
  for(auto* field: subfields) {
    if(eventIDBranchName == field->GetName()) {
      hasEventID = true;
      continue;
    }
    fieldIDs_.emplace_back(field->GetName());
    fieldType.emplace_back(field->GetType());
  }

  for(int laneId=0; laneId < iNLanes; ++laneId) {
    entries_.emplace_back(model->CreateEntry());
    dataProductsPerLane_.emplace_back();
    auto& dataProducts = dataProductsPerLane_.back();
    dataProducts.reserve(fieldIDs_.size());
    delayedReaders_.emplace_back(&queue_, entries_.back().get(),&fieldIDs_);
    auto& delayedReader = delayedReaders_.back();

    auto& addressDataProducts = ptrToDataProducts_[laneId];
    addressDataProducts.reserve(fieldIDs_.size());
    for(int i=0; i< fieldIDs_.size(); ++i) {
      TClass* class_ptr=TClass::GetClass(fieldType[i].c_str());
      addressDataProducts.push_back(entries_.back()->GetRawPtr(fieldIDs_[i]));
      dataProducts.emplace_back(i,
                                &addressDataProducts[i],
                                fieldIDs_[i],
                                class_ptr,
                                &delayedReader);
    }
  }
}

void SerialRNTupleSource::readEventAsync(unsigned int iLane, long iEventIndex, OptionalTaskHolder iTask) {
  if(nEvents_ > iEventIndex) {
    auto temptask = iTask.releaseToTaskHolder();
    auto group = temptask.group();
    queue_.push(*group, [task=std::move(temptask), this, iLane, iEventIndex]() mutable {
        auto start = std::chrono::high_resolution_clock::now();
        events_->LoadEntry(iEventIndex, *entries_[iLane]);
        identifiers_[iLane] = *entries_[iLane]->Get<cce::tf::EventIdentifier>("EventID");
        accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
        task.doneWaiting();
      });
  }
}

std::chrono::microseconds SerialRNTupleSource::accumulatedTime() const {
  auto fullTime = accumulatedTime_;
  for(auto& delayedReader: delayedReaders_) {
    fullTime += delayedReader.accumulatedTime();
  }
  return fullTime;
}

void SerialRNTupleSource::printSummary() const {
  std::chrono::microseconds sourceTime = accumulatedTime();
  std::cout <<"\nSource time: "<<sourceTime.count()<<"us\n"<<std::endl;
}

void SerialRNTupleDelayedRetriever::getAsync(DataProductRetriever& dataProduct, int index, TaskHolder iTask) {
  auto group = iTask.group();
  queue_->push(*group, [&dataProduct, index,this, task = std::move(iTask)]() mutable { 
      auto start = std::chrono::high_resolution_clock::now();
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
        return std::make_unique<SerialRNTupleSource>(iNLanes, iNEvents, *fileName);
    }
    };

  Maker s_maker;
}
