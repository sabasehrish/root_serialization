#if !defined(Waiter_h)
#define Waiter_h

#include <vector>
#include <thread>
#include "EventIdentifier.h"
#include "SerializerWrapper.h"
#include "TaskHolder.h"

class Waiter {
 public:

 Waiter(unsigned int iSerializerIndex, double iScaleFactor):
  scale_{iScaleFactor},
    index_{iSerializerIndex} {}
    void waitAsync(std::vector<SerializerWrapper> const& iSerializers, TaskHolder iCallback) const {
      iCallback.group()->run([iCallback, &iSerializers, scale=scale_, index= index_]() {
	  using namespace std::chrono_literals;
	  auto sleep = scale*iSerializers[index].blob().size()*1us;
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
