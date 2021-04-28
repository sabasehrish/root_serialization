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

    for(auto& seq: sequences_) {
      //seq->Print();

      bufferFile_.ApplySequence(*seq, const_cast<void*>(address));
    }
    //The blob contains the serialized data product
    std::vector<char> blob(bufferFile_.Buffer(), bufferFile_.Buffer()+bufferFile_.Length());
    return blob;
  }

private:
  TBufferFile bufferFile_;
  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> sequences_;
};
}
#endif
