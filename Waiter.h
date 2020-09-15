#if !defined(Waiter_h)
#define Waiter_h

#include <vector>
#include <thread>
#include "EventIdentifier.h"
#include "DataProductRetriever.h"
#include "TaskHolder.h"

class Waiter {
 public:

 Waiter(unsigned int iDataProductIndex, double iScaleFactor):
  scale_{iScaleFactor},
    index_{iDataProductIndex} {}
    void waitAsync(std::vector<DataProductRetriever> const& iRetrievers, TaskHolder iCallback) const {
      iCallback.group()->run([iCallback, &iRetrievers, scale=scale_, index= index_]() {
	  using namespace std::chrono_literals;
	  auto sleep = scale*iRetrievers[index].size()*1us;
	  //std::cout <<"sleep "<<sleep.count()<<std::endl;
	  std::this_thread::sleep_for( sleep);
	  //std::cout <<"awake"<<std::endl;
	});
    }

 private:
  double scale_;
  unsigned int index_;
};
#endif
