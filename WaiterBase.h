#if !defined(WaiterBase_h)
#define WaiterBase_h

#include <vector>
#include <thread>
#include "EventIdentifier.h"
#include "DataProductRetriever.h"
#include "TaskHolder.h"

namespace cce::tf {
class WaiterBase {
 public:

  WaiterBase() = default;
  virtual ~WaiterBase() = default;

  // iLaneIndex is which Lane is being used to process this Event
  // iEventIndex is the index of the event within the Source
  // iProductIndex is which element of iRetrievers is to be waited upon
  virtual void waitAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, long iEventIndex, std::vector<DataProductRetriever> const& iRetrievers, unsigned int iProductIndex, TaskHolder iCallback) const = 0;
};
}
#endif
