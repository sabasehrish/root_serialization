#if !defined(OutputerBase_h)
#define OutputerBase_h

#include <vector>
#include "EventIdentifier.h"
#include "SerializerWrapper.h"
#include "TaskHolder.h"

class DataProductRetriever;

class OutputerBase {
 public:
  virtual ~OutputerBase() = default;
  
  //virtual void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const&, TaskHolder iCallback) const = 0;

  virtual void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers, TaskHolder iCallback) const = 0;
};
#endif
