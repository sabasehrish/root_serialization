#if !defined(common_unrolling_h)
#define common_unrolling_h

#include "TClass.h"
#include "TStreamerInfoActions.h"
#include <memory>
#include <vector>

namespace cce::tf::unrolling {
  using Sequence = std::unique_ptr<TStreamerInfoActions::TActionSequence>;  
  using OffsetAndSequences = std::vector<std::pair<int, Sequence>>;
  OffsetAndSequences buildReadActionSequence(TClass& iClass);
  OffsetAndSequences buildWriteActionSequence(TClass& iClass);


}
#endif
