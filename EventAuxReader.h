#if !defined(EventAuxReader_h)
#define EventAuxReader_h

#include "EventIdentifier.h"
#include "DataFormats/Provenance/interface/EventAuxiliary.h"

class EventAuxReader {
public:
  EventAuxReader(void** iAddress): address_(iAddress){}

  EventIdentifier doWork() {
    auto aux = *reinterpret_cast<edm::EventAuxiliary**>(address_); 
    return EventIdentifier{aux->run(), aux->luminosityBlock(), aux->event()};
  }
private:
  void** address_;
};

#endif
