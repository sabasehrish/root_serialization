#if !defined(UnrolledSerializer_h)
#define UnrolledSerializer_h

#include <vector>
#include "TBufferFile.h"
#include "TClass.h"
#include "TStreamerInfoActions.h"

namespace cce::tf {
class UnrolledSerializer {
public:
  UnrolledSerializer(TClass*);

  //  UnrolledSerializer(Serializer&& ):
  //  bufferFile_{TBuffer::kWrite} {}

  std::vector<char> serialize(void const* address) {
    bufferFile_.Reset();

    bufferFile_.ApplySequence(*sequence_, const_cast<void*>(address));
    //The blob contains the serialized data product
    std::vector<char> blob(bufferFile_.Buffer(), bufferFile_.Buffer()+bufferFile_.Length());
    return blob;
  }

private:

  std::unique_ptr<TStreamerInfoActions::TActionSequence> createForContainer(TVirtualCollectionProxy&) const;
  std::unique_ptr<TStreamerInfoActions::TActionSequence> createForObject(TClass&, TStreamerInfo&) const;
  TBufferFile bufferFile_;
  
  std::unique_ptr<TStreamerInfoActions::TActionSequence> sequence_;
  
};
}
#endif
