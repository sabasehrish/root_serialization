#if !defined(Waiter_h)
#define Waiter_h

#include <vector>
#include <thread>
#include "EventIdentifier.h"
#include "DataProductRetriever.h"
#include "TaskHolder.h"

namespace cce::tf {
class Waiter {
 public:

 Waiter(std::size_t iNumberOfDataProducts,  double iScaleFactor):
  scale_{iScaleFactor} {}

    void waitAsync(std::vector<DataProductRetriever> const& iRetrievers, unsigned int index, TaskHolder iCallback) const {
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
#endif
