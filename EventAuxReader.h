#if !defined(EventAuxReader_h)
#define EventAuxReader_h

#include "EventIdentifier.h"
#include "cms/EventAuxiliary.h"

namespace cce::tf {
class EventAuxReader {
public:
  EventAuxReader(void** iAddress): address_(iAddress){}

  EventIdentifier doWork() {
    if(address_) {
      auto aux = *reinterpret_cast<edm::EventAuxiliary**>(address_); 
      return EventIdentifier{aux->run(), aux->luminosityBlock(), aux->event()};
    } else {
      return EventIdentifier{0,0,0};
    }
  }
private:
  void** address_;
};
}
#endif
