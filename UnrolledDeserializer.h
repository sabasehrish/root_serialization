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

  //  UnrolledDeserializer(Deserializer&& ):
  //  bufferFile_{TBuffer::kWrite} {}

  void* deserialize(std::vector<char> const& iBuffer) {
    bufferFile_.Reset();

    bufferFile_.SetBuffer( const_cast<char*>(&iBuffer.front()), iBuffer.size(), kFALSE);

    void* address = cls_->New();
    for(auto& offNSeq: offsetAndSequences_) {
      //seq->Print();
      bufferFile_.ApplySequence(*(offNSeq.second), static_cast<char*>(address)+offNSeq.first);
    }
    return address;
  }

private:

  TBufferFile bufferFile_;
  TClass* cls_;
  unrolling::OffsetAndSequences offsetAndSequences_;
};
}
#endif
