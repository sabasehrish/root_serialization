#if !defined(UnrolledDeserializer_h)
#define UnrolledDeserializer_h

#include <vector>
#include "TBufferFile.h"
#include "TClass.h"
#include "TStreamerInfoActions.h"
#include "common_unrolling.h"

namespace cce::tf {
class UnrolledDeserializer {
public:
  UnrolledDeserializer(TClass*);


  int deserialize(std::vector<char> const& iBuffer, void* iWriteTo) const {
    return deserialize(&iBuffer.front(), iBuffer.size(), iWriteTo);
  }
  int deserialize(char const * iBuffer, size_t iBufferSize, void* iWriteTo) const{
    TBufferFile bufferFile{TBuffer::kRead};

    bufferFile.SetBuffer( const_cast<char*>(iBuffer), iBufferSize, kFALSE);

    void* address = iWriteTo;
    for(auto& offNSeq: offsetAndSequences_.m_objects) {
      //seq->Print();
      bufferFile.ApplySequence(*(offNSeq.second), static_cast<char*>(address)+offNSeq.first);
    }

    for(auto& coll: offsetAndSequences_.m_collections) {
      auto collAddress = static_cast<char const*>(address) + coll.m_offset;

      TVirtualCollectionProxy::TPushPop helper(coll.m_collProxy.get(), const_cast<char*>(collAddress));
      Int_t size;
      bufferFile >> size;      
      coll.m_collProxy->Allocate(size, true);

      for(Int_t item=0; item<size; ++item) {
        auto elementAddress = (*coll.m_collProxy)[item];
        for(auto& offNSeq: coll.m_offsetAndSequences) {
          //seq->Print();
          bufferFile.ApplySequence(*(offNSeq.second), elementAddress+offNSeq.first);
        }
      }
    }
    return bufferFile.Length();
  }

private:
  unrolling::ObjectAndCollectionsSequences offsetAndSequences_;
};
}
#endif
