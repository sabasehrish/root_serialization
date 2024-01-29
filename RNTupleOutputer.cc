#include <iostream>
#include "RNTupleOutputer.h"
#include "OutputerFactory.h"
#include "FunctorTask.h"

#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RField.hxx>
#include <ROOT/RFieldVisitor.hxx>

using namespace cce::tf;


void RNTupleOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  if ( iLaneIndex == 0 ) {
    auto model = ROOT::Experimental::RNTupleModel::Create();
    for(auto const& dp: iDPs) {
      // chop last . if present
      auto name = dp.name().substr(0, dp.name().find("."));
      if ( verbose_ > 1 ) std::cout << "-------- Creating field for " << name << " of type " << dp.classType()->GetName() << "\n";
      try { 
        auto field = ROOT::Experimental::Detail::RFieldBase::Create(name, dp.classType()->GetName()).Unwrap();
        if ( verbose_ > 1 ) ROOT::Experimental::RPrintSchemaVisitor(std::cout, '*', 1000, 10).VisitField(*field);
        model->AddField(std::move(field));
      }
      catch (ROOT::Experimental::RException& e) {
        if ( verbose_ > 1 ) std::cout << "Failed: " << e.what() << "\n";
      }
    }
    // https://root.cern/doc/v626/classROOT_1_1Experimental_1_1RNTupleWriteOptions.html
    ntuple_ = ROOT::Experimental::RNTupleWriter::Recreate(std::move(model), "Events", fileName_);
  }
  else if ( not ntuple_ ) {
    throw std::logic_error("setupForLane should be sequential");
  }
  // would be nice to have ntuple_->CreateEntry(field_map) or such
  for(auto const& dp: iDPs) {
    auto name = dp.name().substr(0, dp.name().find("."));
    auto* field = ntuple_->GetModel()->GetField(name);
    entries_[iLaneIndex].fields.push_back(field);
    entries_[iLaneIndex].ptrs.push_back(dp.address());
    if ( field == nullptr ) continue;
    // entries_[iLaneIndex].entry.CaptureValue(field->CaptureValue(dp.address()));
  }
}

void RNTupleOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
  assert(iLaneIndex < entries_.size());
  auto group = iCallback.group();
  auto* field = entries_[iLaneIndex].fields[iDataProduct.index()];
  // can we do field->CaptureValue(ptr) here?
}

void RNTupleOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto group = iCallback.group();
  collateQueue_.push(*group, [this, iEventID, iLaneIndex, callback=std::move(iCallback)]() mutable {
      collateProducts(iEventID, entries_[iLaneIndex], std::move(callback));
    });
  auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  parallelTime_ += time.count();
}

void RNTupleOutputer::printSummary() const {
  std::chrono::microseconds serializerTime = std::chrono::microseconds::zero();
  for(const auto& lane : entries_) {
    serializerTime += lane.accumulatedTime();
  }
  std::cout <<"RNTupleOutputer\n"
    "  total serial collate time at end event: "<<collateTime_.count()<<"us\n"
    "  total non-serializer parallel time at end event: "<<parallelTime_.load()<<"us\n"
    "  total serializer parallel time at end event: "<<serializerTime.count()<<"us\n";
}

void RNTupleOutputer::collateProducts(
    EventIdentifier const& iEventID,
    RNTupleOutputer::EntryContainer& entry,
    TaskHolder iCallback
    ) const
{
  auto start = std::chrono::high_resolution_clock::now();
  auto thisOffset = eventGlobalOffset_++;
  // iEventID.run
  // iEventID.lumi
  // iEventID.event
  if ( verbose_ > 0 ) std::cout << thisOffset << " event id " << iEventID.run << "\n";

  auto rentry = ntuple_->CreateEntry();
  for(size_t i=0; i < entry.fields.size(); ++i) {
    auto* field = entry.fields[i];
    void** ptr = entry.ptrs[i];
    if ( field == nullptr ) continue;
    rentry->CaptureValueUnsafe(field->GetName(), *ptr);
  }
  ntuple_->Fill(*rentry);

  collateTime_ += std::chrono::duration_cast<decltype(collateTime_)>(std::chrono::high_resolution_clock::now() - start);
}


namespace {
class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("RNTupleOutputer") {}
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {
      auto verbose = params.get<int>("verbose", 0);
      auto fileName = params.get<std::string>("fileName");
      if(not fileName) {
        std::cerr << "no filename given for RNTupleOutputer\n";
        return {};
      }
      return std::make_unique<RNTupleOutputer>(iNLanes, fileName.value(), verbose);
    }
};

Maker s_maker;
}
