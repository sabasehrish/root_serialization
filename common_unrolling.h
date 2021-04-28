#if !defined(common_unrolling_h)
#define common_unrolling_h

#include "TClass.h"
#include "TStreamerInfo.h"
#include "TStreamerInfoActions.h"
#include <memory>

namespace cce::tf::unrolling {
  TStreamerInfo* buildStreamerInfo(TClass* cl);
  TStreamerInfo* buildStreamerInfo(TClass* cl, void* pointer);
  void checkIfCanHandle(TClass* iClass);
  bool elementNeedsOwnSequence(TStreamerElement* element, TClass* cl);
  bool canUnroll(TClass* iClass, TStreamerInfo* sinfo);

  std::unique_ptr<TStreamerInfoActions::TActionSequence> setActionSequence(TClass *originalClass, TStreamerInfo *localInfo, TVirtualCollectionProxy* collectionProxy, TStreamerInfoActions::TActionSequence::SequenceGetter_t create, bool isSplitNode, int iID, size_t iOffset );
}
#endif
