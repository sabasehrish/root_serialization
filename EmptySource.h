#if !defined(EmptySource_h)
#define EmptySource_h

#include "SourceBase.h"

namespace cce::tf {
class EmptySource : public SourceBase {
 public:
  explicit EmptySource():
  SourceBase() {}

  std::vector<DataProductRetriever>& dataProducts() final { return empty_;}
  EventIdentifier eventIdentifier() final {
    return {1, 1, index_};
  }

 private:
  bool readEvent(long ) final {++index_; return true; }
  unsigned long long index_ = 0;
  std::vector<DataProductRetriever> empty_;
};
}
#endif
