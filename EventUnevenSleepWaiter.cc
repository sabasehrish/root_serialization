
#include <vector>
#include <fstream>
#include <thread>
#include <iostream>
#include "EventIdentifier.h"
#include "DataProductRetriever.h"
#include "TaskHolder.h"
#include "WaiterBase.h"
#include "WaiterFactory.h"


namespace cce::tf {
  class EventUnevenSleepWaiter : public WaiterBase {
 public:

    EventUnevenSleepWaiter(std::vector<double> iEventSleepTimes, unsigned int iDivideBetween, std::size_t iNDataProducts):
      sleepTimes_(std::move(iEventSleepTimes)),
      divideBetween_(iDivideBetween),
      nDataProducts_{iNDataProducts} {}

    void waitAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, long iEventIndex,
                   std::vector<DataProductRetriever> const& iRetrievers, unsigned int index, 
                   TaskHolder iCallback) const final {
      if(index < divideBetween_) {
        iCallback.group()->run([iCallback, iEventIndex, this]() {
            using namespace std::chrono_literals;
            auto index = iEventIndex % sleepTimes_.size();
            auto sleep = (sleepTimes_[index]/divideBetween_)*1us;
            //std::cout <<"sleep "<<sleep.count()<<std::endl;
            std::this_thread::sleep_for( sleep);
            //std::cout <<"awake"<<std::endl;
          });
      }
    }

 private:
    std::vector<double> sleepTimes_;
    unsigned int divideBetween_;
    std::size_t nDataProducts_;
};
}

namespace {

  using namespace cce::tf;
  class Maker : public WaiterMakerBase {
  public:
    Maker(): WaiterMakerBase("EventUnevenSleepWaiter") {}

    std::unique_ptr<WaiterBase> create(unsigned int iNLanes, std::size_t iNDataProducts, ConfigurationParameters const& params) const final {
      auto scale = params.get<float>("scale", 1.);

      auto filename = params.get<std::string>("filename");
      if(not filename) {
        std::cout <<"no file name give for EventUnevenSleepWaiter"<<std::endl;
        return {};
      }

      std::ifstream file(*filename);
      if(not file.is_open()) {
        std::cout <<"unable to open file "<<*filename<<" with sleep times";
        return {};
      }
      double value;
      std::vector<double> sleepTimes;
      while(file >> value) {
        sleepTimes.push_back(value*scale);
      }
      if(sleepTimes.empty()) {
        std::cout <<"file "<<*filename<<" contained no event times"<<std::endl;
        return {};
      }

      int divideBetween = params.get<int>("divideBetween",0);
      if(0 == divideBetween) {
        divideBetween = iNDataProducts;
      }
      if(divideBetween > iNDataProducts) {
        std::cout <<"value of 'divideBetween' "<<divideBetween<<" is greater than number of products "<<iNDataProducts<<std::endl;
        return {};
      }     

      return std::make_unique<EventUnevenSleepWaiter>(std::move(sleepTimes), divideBetween, iNDataProducts);
    }
    
  };

  Maker s_maker;
}
