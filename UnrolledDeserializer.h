#if !defined(UnrolledDeserializer_h)
#define UnrolledDeserializer_h

#include <vector>
#include "TBufferFile.h"
#include "TClass.h"
#include "TStreamerInfoActions.h"

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
    for(auto& seq: sequences_) {
      //seq->Print();
      bufferFile_.ApplySequence(*seq, address);
    }
    return address;
  }

private:

  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> createUnrolled(TClass&, TStreamerInfo&) const;
  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> createRolled(TClass&, TStreamerInfo&) const;
  TBufferFile bufferFile_;
  TClass* cls_;
  
  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> sequences_;
  
};
}
#endif
