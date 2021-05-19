#ifndef canvas_Persistency_Provenance_EventAuxiliary_h
#define canvas_Persistency_Provenance_EventAuxiliary_h
// vim: set sw=2 expandtab :

#include "EventID.h"
#include "Timestamp.h"

namespace art {
  class EventAuxiliary {
  public:
    enum ExperimentType {
      Any = 0,
      Align = 1,
      Calib = 2,
      Cosmic = 3,
      Data = 4,
      Mc = 5,
      Raw = 6,
      Test = 7
    };

    EventAuxiliary();

    RunNumber_t run() const noexcept { return id_.run(); }
    SubRunNumber_t subRun() const noexcept { return id_.subRun(); }
    EventNumber_t event() const noexcept { return id_.event(); }

  private:
    EventID id_{};
    Timestamp time_{}; // Unused
    bool isRealData_{false}; // Unused
    ExperimentType experimentType_{Any}; // Unused
  };
} // namespace art

#endif /* canvas_Persistency_Provenance_EventAuxiliary_h */

// Local Variables:
// mode: c++
// End:
