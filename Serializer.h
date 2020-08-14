#if !defined(Serializer_h)
#define Serializer_h

#include <vector>
#include "TBufferFile.h"
#include "TClass.h"

class Serializer {
public:
  Serializer() : bufferFile_{TBuffer::kWrite} {}

  Serializer(Serializer&& ):
    bufferFile_{TBuffer::kWrite} {}

  Serializer(Serializer const&):
    bufferFile_{TBuffer::kWrite} {}

  std::vector<char> serialize(void* address, TClass* tClass) {
    bufferFile_.Reset();
    tClass->WriteBuffer(bufferFile_, address);
    //The blob contains the serialized data product
    std::vector<char> blob(bufferFile_.Buffer(), bufferFile_.Buffer()+bufferFile_.Length());
    return blob;
  }

private:
  TBufferFile bufferFile_;
};

#endif
