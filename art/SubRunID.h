#ifndef canvas_Persistency_Provenance_SubRunID_h
#define canvas_Persistency_Provenance_SubRunID_h

// A SubRunID represents a unique period within a run.

#include "RunID.h"

namespace art {

  class SubRunID {
  public:
    RunNumber_t run() const { return run_.run(); }
    SubRunNumber_t subRun() const { return subRun_; }

  private:
    RunID run_{};
    SubRunNumber_t subRun_{IDNumber<Level::SubRun>::invalid()};
  };
}

#endif /* canvas_Persistency_Provenance_SubRunID_h */

// Local Variables:
// mode: c++
// End:
