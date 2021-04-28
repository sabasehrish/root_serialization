#if !defined(common_unrolling_h)
#define common_unrolling_h

#include "TClass.h"
#include "TStreamerInfoActions.h"
#include <memory>
#include <vector>

namespace cce::tf::unrolling {
  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> buildReadActionSequence(TClass& iClass);
  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> buildWriteActionSequence(TClass& iClass);


}
#endif
