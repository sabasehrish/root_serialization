#ifndef canvas_Persistency_Provenance_RunID_h
#define canvas_Persistency_Provenance_RunID_h

//
// A RunID represents a unique period of operation of the data
// acquisition system, which we call a "run".
//
// Each RunID contains a fixed-size unsigned integer, the run number.
//

#include "IDNumber.h"

namespace art {
  class RunID {
  public:
    RunNumber_t run() const { return run_; }

  private:
    RunNumber_t run_{IDNumber<Level::Run>::invalid()};
  };
} // namespace art

#endif /* canvas_Persistency_Provenance_RunID_h */

// Local Variables:
// mode: c++
// End:
