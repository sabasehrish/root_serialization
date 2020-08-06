#if !defined(OutputerBase_h)
#define OutputerBase_h

#include <vector>
#include "EventIdentifier.h"
#include "SerializerWrapper.h"
#include "TaskHolder.h"

class OutputerBase {
 public:
  virtual ~OutputerBase() = default;

  virtual void outputAsync(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers, TaskHolder iCallback) const = 0;
};
#endif
