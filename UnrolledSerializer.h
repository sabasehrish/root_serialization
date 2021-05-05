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

    for(auto& offAndSeq: offsetAndSequences_.m_objects) {
      //seq->Print();
      bufferFile_.ApplySequence(*(offAndSeq.second), const_cast<char*>(static_cast<char const*>(address)+offAndSeq.first));
    }

    for(auto& coll: offsetAndSequences_.m_collections) {
      auto collAddress = static_cast<char const*>(address) + coll.m_offset;

      TVirtualCollectionProxy::TPushPop helper(coll.m_collProxy.get(), const_cast<char*>(collAddress));
      Int_t size =coll.m_collProxy->Size();
      bufferFile_ << size;

      for(Int_t item=0; item<size; ++item) {
        auto elementAddress = (*coll.m_collProxy)[item];
        for(auto& offAndSeq: coll.m_offsetAndSequences) {
          //seq->Print();
          bufferFile_.ApplySequence(*(offAndSeq.second), elementAddress+offAndSeq.first);
        }
      }
    }
    //The blob contains the serialized data product
    std::vector<char> blob(bufferFile_.Buffer(), bufferFile_.Buffer()+bufferFile_.Length());
    return blob;
  }

private:
  TBufferFile bufferFile_;
  unrolling::ObjectAndCollectionsSequences offsetAndSequences_;
};
}
#endif
