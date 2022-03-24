
#include <vector>
#include <thread>
#include "EventIdentifier.h"
#include "DataProductRetriever.h"
#include "TaskHolder.h"
#include "WaiterBase.h"
#include "WaiterFactory.h"


namespace cce::tf {
  class ScaleWaiter : public WaiterBase {
 public:

 ScaleWaiter(std::size_t iNumberOfDataProducts,  double iScaleFactor):
  scale_{iScaleFactor} {}

    void waitAsync(std::vector<DataProductRetriever> const& iRetrievers, unsigned int index, TaskHolder iCallback) const final {
      iCallback.group()->run([iCallback, &iRetrievers, scale=scale_, index]() {
	  using namespace std::chrono_literals;
	  auto sleep = scale*iRetrievers[index].size()*1us;
	  //std::cout <<"sleep "<<sleep.count()<<std::endl;
	  std::this_thread::sleep_for( sleep);
	  //std::cout <<"awake"<<std::endl;
	});
    }

 private:
  double scale_;
};
}

namespace {

  using namespace cce::tf;
  class Maker : public WaiterMakerBase {
  public:
    Maker(): WaiterMakerBase("ScaleWaiter") {}

    std::unique_ptr<WaiterBase> create(std::size_t iNDataProducts, ConfigurationParameters const& params) const final {

      auto scale = params.get<float>("scale", 0);

      return std::make_unique<ScaleWaiter>(iNDataProducts, scale);
    }
    
  };

  Maker s_maker;
}
