#ifndef canvas_Persistency_Provenance_EventID_h
#define canvas_Persistency_Provenance_EventID_h

// An EventID labels an unique readout of the data acquisition system,
// which we call an "event".

#include "SubRunID.h"

#include <cstdint>

namespace art {
  class EventID {
  public:
    RunNumber_t run() const { return subRun_.run(); }
    SubRunNumber_t subRun() const { return subRun_.subRun(); }
    EventNumber_t event() const { return event_; }

  private:
    SubRunID subRun_{};
    EventNumber_t event_{IDNumber<Level::Event>::invalid()};
  };
}

#endif /* canvas_Persistency_Provenance_EventID_h */

// Local Variables:
// mode: c++
// End:
