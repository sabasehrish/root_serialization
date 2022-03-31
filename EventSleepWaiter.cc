
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
  class EventSleepWaiter : public WaiterBase {
 public:

    EventSleepWaiter(std::vector<double> iEventSleepTimes, std::size_t iNDataProducts):
      sleepTimes_(std::move(iEventSleepTimes)),
      nDataProducts_{iNDataProducts} {}

    void waitAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, long iEventIndex,
                   std::vector<DataProductRetriever> const& iRetrievers, unsigned int index, 
                   TaskHolder iCallback) const final {
      iCallback.group()->run([iCallback, iEventIndex, this]() {
	  using namespace std::chrono_literals;
          auto index = iEventIndex % sleepTimes_.size();
	  auto sleep = (sleepTimes_[index]/nDataProducts_)*1us;
	  //std::cout <<"sleep "<<sleep.count()<<std::endl;
	  std::this_thread::sleep_for( sleep);
	  //std::cout <<"awake"<<std::endl;
	});
    }

 private:
    std::vector<double> sleepTimes_;
    std::size_t nDataProducts_;
};
}

namespace {

  using namespace cce::tf;
  class Maker : public WaiterMakerBase {
  public:
    Maker(): WaiterMakerBase("EventSleepWaiter") {}

    std::unique_ptr<WaiterBase> create(unsigned int iNLanes, std::size_t iNDataProducts, ConfigurationParameters const& params) const final {

      auto filename = params.get<std::string>("filename");
      if(not filename) {
        std::cout <<"no file name give for EventSleepWaiter"<<std::endl;
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
        sleepTimes.push_back(value);
      }
      if(sleepTimes.empty()) {
        std::cout <<"file "<<*filename<<" contained no event times"<<std::endl;
        return {};
      }

      return std::make_unique<EventSleepWaiter>(std::move(sleepTimes), iNDataProducts);
    }
    
  };

  Maker s_maker;
}
