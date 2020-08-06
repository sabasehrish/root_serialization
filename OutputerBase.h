#if !defined(OutputerBase_h)
#define OutputerBase_h

#include <vector>
#include "EventIdentifier.h"
#include "SerializerWrapper.h"

class OutputerBase {
 public:
  virtual ~OutputerBase() = default;

  virtual void output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) const = 0;
};
#endif
