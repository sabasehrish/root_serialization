#if !defined(DelayedProductRetriever_h)
#define DelayedProductRetriever_h

#include "TaskHolder.h"


namespace cce::tf {
class DelayedProductRetriever {
 public:
  virtual ~DelayedProductRetriever() {}

  virtual void getAsync(int index, TaskHolder iCallback) = 0;
};
}
#endif
