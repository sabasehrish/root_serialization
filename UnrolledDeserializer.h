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
    bufferFile_.ApplySequence(*sequence_, address);
    return address;
  }

private:

  std::unique_ptr<TStreamerInfoActions::TActionSequence> createForContainer(TVirtualCollectionProxy&) const;
  std::unique_ptr<TStreamerInfoActions::TActionSequence> createForObject(TClass&, TStreamerInfo&) const;
  TBufferFile bufferFile_;
  TClass* cls_;
  
  std::unique_ptr<TStreamerInfoActions::TActionSequence> sequence_;
  
};
}
#endif
