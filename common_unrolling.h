#if !defined(common_unrolling_h)
#define common_unrolling_h

#include "TClass.h"
#include "TStreamerInfoActions.h"
#include <memory>
#include <vector>

namespace cce::tf::unrolling {
  using Sequence = std::unique_ptr<TStreamerInfoActions::TActionSequence>;  
  using OffsetAndSequences = std::vector<std::pair<int, Sequence>>;

  struct CollectionActions {
  CollectionActions( TVirtualCollectionProxy* proxy, int offset): 
    m_collProxy(proxy), m_offset(offset) {}

    std::unique_ptr<TVirtualCollectionProxy> m_collProxy;
    int m_offset;
    OffsetAndSequences m_offsetAndSequences;

    std::vector<CollectionActions> m_collections;
  };

  using SequencesForCollections = std::vector<CollectionActions>;

  struct ObjectAndCollectionsSequences {
    OffsetAndSequences m_objects;
    SequencesForCollections m_collections;
  };

  ObjectAndCollectionsSequences buildReadActionSequence(TClass& iClass);
  ObjectAndCollectionsSequences buildWriteActionSequence(TClass& iClass);


}
#endif
