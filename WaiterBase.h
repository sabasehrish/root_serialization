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

  virtual void waitAsync(std::vector<DataProductRetriever> const& iRetrievers, unsigned int index, TaskHolder iCallback) const = 0;
};
}
#endif
