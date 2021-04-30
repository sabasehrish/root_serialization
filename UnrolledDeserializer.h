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
    for(auto& offNSeq: offsetAndSequences_) {
      //seq->Print();
      bufferFile.ApplySequence(*(offNSeq.second), static_cast<char*>(address)+offNSeq.first);
    }
    return bufferFile.Length();
  }

private:
  unrolling::OffsetAndSequences offsetAndSequences_;
};
}
#endif
