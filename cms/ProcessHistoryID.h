#ifndef DataFormats_Provenance_ProcessHistoryID_h
#define DataFormats_Provenance_ProcessHistoryID_h

#include "HashedTypes.h"
#include "Hash.h"

namespace edm {
  typedef Hash<ProcessHistoryType> ProcessHistoryID;
}

#endif
