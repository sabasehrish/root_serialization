#if !defined(EventAuxReader_h)
#define EventAuxReader_h

#include <cassert>

#include "EventIdentifier.h"
#include "cms/EventAuxiliary.h"

namespace cce::tf {

inline EventIdentifier cmsEventID(void** address) { 
   assert(address);
   auto aux = *reinterpret_cast<edm::EventAuxiliary**>(address); 
   return EventIdentifier{aux->run(), aux->luminosityBlock(), aux->event()};
}

/*
inline EventIdentifier artEventID(void** address) {
  assert(address);
  auto aux = *reinterpret_cast<art::EventAuxiliary**>(address);
  return EventIdentifier{aux->run(), aux->subRun(), aux->event()};
}
*/

class EventAuxReader {
public:
  EventAuxReader(std::function<EventIdentifier()> f={}): func_(std::move(f)){}

  EventIdentifier doWork() {
    if(func_) return func_();
    return EventIdentifier{0,0,0};
    

  }
private:
  std::function<EventIdentifier()> func_;
};
}
#endif
