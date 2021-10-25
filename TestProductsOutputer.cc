#include "TestProductsOutputer.h"
#include "DataProductRetriever.h"
#include "OutputerFactory.h"

#include <vector>
#include <iostream>
#include <cstdlib>

using namespace cce::tf;

TestProductsOutputer::TestProductsOutputer(unsigned int iNLanes):
  retrieverPerLane_(iNLanes) {}

void TestProductsOutputer::setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iRetrievers) {
  retrieverPerLane_[iLaneIndex] = &iRetrievers;
}

void TestProductsOutputer::productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iRetriever, TaskHolder iCallback) const {
  iCallback.doneWaiting();
}

bool TestProductsOutputer::usesProductReadyAsync() const {
  return false;
}


void TestProductsOutputer::outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const {
  auto const& retrievers = *retrieverPerLane_[iLaneIndex];

  auto const index = iEventID.event -1;
  for(auto const& prod: retrievers) {
    if(prod.name() == "ints") {
      std::vector<int>* ints = reinterpret_cast<std::vector<int>*>(*prod.address());
      if(ints->size() != 3) {
        std::cout <<"ERROR: ints has incorrect size "<<ints->size()<<" in event "<<iEventID.event <<std::endl;
        abort();
      }
      for(int i=0; i<3; ++i) {
        if( (*ints)[i] != index + i) {
          std::cout <<"ERROR: ints index "<<i<<" has wrong value "<<(*ints)[i]<<" in event "<<iEventID.event<<std::endl;
          abort();
        }
      }
    } else if(prod.name() == "floats") {
      std::vector<float>* floats = reinterpret_cast<std::vector<float>*>(*prod.address());
      if(floats->size() != 3) {
        std::cout <<"ERROR: floats has incorrect size "<<floats->size()<<" in event "<<iEventID.event<<std::endl;
        abort();
      }
      for(int i=0; i<3; ++i) {
        if( (*floats)[i] != index*(i+1.f)) {
          std::cout <<"ERROR: floats index "<<i<<" has wrong value "<<(*floats)[i]<<" in event "<<iEventID.event<<std::endl;
          abort();
        }
      }
    } else {
      std::cout <<"ERROR: unknown product name "<<prod.name()<<" in event "<<iEventID.event<<std::endl;
      abort();
    }
  }

  iCallback.doneWaiting();
}

void TestProductsOutputer::printSummary() const {
  std::cout <<"\nOutputer time: N/A"<<std::endl;
}

namespace {
    class TestProductsMaker : public OutputerMakerBase {
  public:
    TestProductsMaker(): OutputerMakerBase("TestProductsOutputer") {}
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params, int) const final {
      return std::make_unique<TestProductsOutputer>(iNLanes);
    }
    };

  TestProductsMaker s_maker;
}
