#include <iostream>
#include "RNTupleTFileOutputer.h"
#include "OutputerFactory.h"
#include "FunctorTask.h"
#include "RNTupleOutputerConfig.h"

#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RField.hxx>
#include <ROOT/RFieldVisitor.hxx>

using namespace cce::tf;

RNTupleTFileOutputer::RNTupleTFileOutputer(std::string const& fileName, unsigned int iNLanes, RNTupleOutputerConfig const& iConfig):
    fileName_(fileName),
    config_(iConfig),
    file_(fileName.c_str(), "recreate", ""),
    entries_(iNLanes),
    collateTime_{std::chrono::microseconds::zero()},
    parallelTime_{0}
  { }

void RNTupleTFileOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) {
  if ( iLaneIndex == 0 ) {
    const std::string eventAuxiliaryBranchName{"EventAuxiliary"}; 
    bool hasEventAuxiliaryBranch = false;
    
    auto model = ROOT::RNTupleModel::CreateBare();
    fieldIDs_.reserve(iDPs.size());
    for(auto const& dp: iDPs) {
      // chop last . if present
      if(dp.name() == eventAuxiliaryBranchName) {
        hasEventAuxiliaryBranch = true;
      }
      auto name = dp.name().substr(0, dp.name().find("."));
      if ( config_.verbose_ > 1 ) std::cout << "-------- Creating field for " << name << " of type " << dp.classType()->GetName() << "\n";
      try { 
        auto field = ROOT::RFieldBase::Create(name, dp.classType()->GetName()).Unwrap();
        assert(field);
        if ( config_.verbose_ > 1 ) ROOT::Internal::RPrintSchemaVisitor(std::cout, '*', 1000, 10).VisitField(*field);
        model->AddField(std::move(field));
        fieldIDs_.emplace_back(std::move(name));
        
      }
      catch (ROOT::RException& e) {
         std::cout << "Failed: " << e.what() << "\n";
         throw std::runtime_error("Failed to create field");
      }
    }
    if(not hasEventAuxiliaryBranch) {
      id_ = std::make_shared<EventIdentifier>();
      auto field = ROOT::RFieldBase::Create("EventID", "cce::tf::EventIdentifier").Unwrap();
      if ( config_.verbose_ > 1 ) ROOT::Internal::RPrintSchemaVisitor(std::cout, '*', 1000, 10).VisitField(*field);
      assert(field);
      model->AddField(std::move(field));
      fieldIDs_.emplace_back("EventID");
    }
    // https://root.cern/doc/v626/classROOT_1_1Experimental_1_1RNTupleWriteOptions.html
    auto writeOptions = ROOT::RNTupleWriteOptions();
    writeOptions.SetCompression(config_.compressionAlgorithm_, config_.compressionLevel_);
    writeOptions.SetMaxUnzippedPageSize(config_.maxUnzippedPageSize_);
    writeOptions.SetApproxZippedClusterSize(config_.approxZippedClusterSize_);
    writeOptions.SetMaxUnzippedClusterSize(config_.maxUnzippedClusterSize_);
    writeOptions.SetUseBufferedWrite(config_.useBufferedWrite_);
    
    ntuple_ = ROOT::RNTupleWriter::Append(std::move(model), "Events", file_, writeOptions);
  }
  else if ( not ntuple_ ) {
    throw std::logic_error("setupForLane should be sequential");
  }
  entries_[iLaneIndex].retrievers = &iDPs;
}

void RNTupleTFileOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const {
}

void RNTupleTFileOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto start = std::chrono::high_resolution_clock::now();
  auto group = iCallback.group();
  collateQueue_.push(*group, [this, iEventID, iLaneIndex, callback=std::move(iCallback)]() mutable {
      collateProducts(iEventID, entries_[iLaneIndex], std::move(callback));
    });
  auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  parallelTime_ += time.count();
}

void RNTupleTFileOutputer::printSummary() const {
  auto start = std::chrono::high_resolution_clock::now();
  ntuple_.reset();
  auto deleteTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

  start = std::chrono::high_resolution_clock::now();

  std::cout <<"RNTupleTFileOutputer\n"
    "  total serial collate time at end event: "<<collateTime_.count()<<"us\n"
    "  total non-serializer parallel time at end event: "<<parallelTime_.load()<<"us\n"
    "  end of job RNTupleWriter shutdown time: "<<deleteTime.count()<<"us\n";
}

void RNTupleTFileOutputer::collateProducts(
    EventIdentifier const& iEventID,
    RNTupleTFileOutputer::EntryContainer const& entry,
    TaskHolder iCallback
    ) const
{
  auto start = std::chrono::high_resolution_clock::now();
  auto thisOffset = eventGlobalOffset_++;
  if ( config_.verbose_ > 0 ) std::cout << thisOffset << " event id " << iEventID.run << ", "<< iEventID.lumi<<", "<<iEventID.event<<"\n";

  auto rentry = ntuple_->CreateEntry();
  for(size_t i=0; i < entry.retrievers->size(); ++i) {
    void** ptr = (*entry.retrievers)[i].address();
    rentry->BindRawPtr(fieldIDs_[i], *ptr);
  }
  if(id_) {
    *id_ = iEventID;
    rentry->BindRawPtr("EventID", id_.get());
  }
  ntuple_->Fill(*rentry);

  collateTime_ += std::chrono::duration_cast<decltype(collateTime_)>(std::chrono::high_resolution_clock::now() - start);
}


namespace {
class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("RNTupleTFileOutputer") {}
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {

      auto result = parseRNTupleConfig(params);
      if(not result) {
        return {};
      }
      return std::make_unique<RNTupleTFileOutputer>(result->first, iNLanes, result->second);
    }
};

Maker s_maker;
}
