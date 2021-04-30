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

    for(auto& offAndSeq: offsetAndSequences_) {
      //seq->Print();

      bufferFile_.ApplySequence(*(offAndSeq.second), const_cast<char*>(static_cast<char const*>(address)+offAndSeq.first));
    }
    //The blob contains the serialized data product
    std::vector<char> blob(bufferFile_.Buffer(), bufferFile_.Buffer()+bufferFile_.Length());
    return blob;
  }

private:
  TBufferFile bufferFile_;
  unrolling::OffsetAndSequences offsetAndSequences_;
};
}
#endif
