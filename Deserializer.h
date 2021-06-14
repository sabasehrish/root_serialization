#if !defined(Deserializer_h)
#define Deserializer_h

#include <vector>
#include "TBufferFile.h"
#include "TClass.h"

namespace cce::tf {
class Deserializer {
public:
 Deserializer(TClass* iClass) : class_{iClass} {}


  int deserialize(std::vector<char> const& iBuffer, void* iWriteTo) const {
    return deserialize(&iBuffer.front(), iBuffer.size(), iWriteTo);
  }
  int deserialize(char const * iBuffer, size_t iBufferSize, void* iWriteTo) const{
    TBufferFile bufferFile{TBuffer::kRead};

    bufferFile.SetBuffer( const_cast<char*>(iBuffer), iBufferSize, kFALSE);

    class_->ReadBuffer(bufferFile, iWriteTo);
    return bufferFile.Length();
  }

private:
  TClass* class_;
};
}
#endif
