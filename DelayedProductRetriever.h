#if !defined(DelayedProductRetriever_h)
#define DelayedProductRetriever_h

#include "TaskHolder.h"


namespace cce::tf {
class DataProductRetriever;

class DelayedProductRetriever {
 public:
  virtual ~DelayedProductRetriever() {}

  virtual void getAsync(DataProductRetriever&, int index, TaskHolder iCallback) = 0;
};
}
#endif
