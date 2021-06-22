#if !defined(UnrolledSerializer_h)
#define UnrolledSerializer_h

#include <vector>
#include "TBufferFile.h"
#include "TClass.h"
#include "TStreamerInfoActions.h"
#include "common_unrolling.h"

namespace cce::tf {
class UnrolledSerializer {
public:
  UnrolledSerializer(TClass*);

  UnrolledSerializer(UnrolledSerializer&& iOther):
  bufferFile_{TBuffer::kWrite}, offsetAndSequences_(std::move(iOther.offsetAndSequences_))  {}
  
  UnrolledSerializer(UnrolledSerializer const& ) = delete;

  std::vector<char> serialize(void const* address) {
    bufferFile_.Reset();

    serialize(address, offsetAndSequences_.m_objects, offsetAndSequences_.m_collections);

    //The blob contains the serialized data product
    std::vector<char> blob(bufferFile_.Buffer(), bufferFile_.Buffer()+bufferFile_.Length());
    return blob;
  }

private:
  void serialize(void const* address, unrolling::OffsetAndSequences& offsetAndSequences, unrolling::SequencesForCollections& seq4Collections) {
    for(auto& offAndSeq: offsetAndSequences) {
      //seq->Print();
      bufferFile_.ApplySequence(*(offAndSeq.second), const_cast<char*>(static_cast<char const*>(address)+offAndSeq.first));
    }

    for(auto& coll: seq4Collections) {
      auto collAddress = static_cast<char const*>(address) + coll.m_offset;

      TVirtualCollectionProxy::TPushPop helper(coll.m_collProxy.get(), const_cast<char*>(collAddress));
      Int_t size =coll.m_collProxy->Size();
      bufferFile_ << size;

      for(Int_t item=0; item<size; ++item) {
        auto elementAddress = (*coll.m_collProxy)[item];
        serialize(elementAddress, coll.m_offsetAndSequences, coll.m_collections);
      }
    }
  }

  TBufferFile bufferFile_;
  unrolling::ObjectAndCollectionsSequences offsetAndSequences_;
};
}
#endif
