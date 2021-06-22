#if !defined(DeserializeStrategy_h)
#define DeserializeStrategy_h

#include "TaskHolder.h"

#include <vector>
#include "TClass.h"

#include "ProxyVector.h"

namespace cce::tf {
class DeserializeProxyBase {
 public:
 DeserializeProxyBase() = default;

 virtual ~DeserializeProxyBase();

  virtual int deserialize(std::vector<char> const& iBuffer, void* iWriteTo) const = 0;
  virtual int deserialize(char const * iBuffer, size_t iBufferSize, void* iWriteTo) const = 0;
};


template<typename D>
class DeserializeProxy final : public DeserializeProxyBase {
 public:
 DeserializeProxy(TClass* tClass):
  deserializer_{tClass} {}

  int deserialize(std::vector<char> const& iBuffer, void* iWriteTo) const {
    return deserializer_.deserialize(iBuffer, iWriteTo);
  }
  int deserialize(char const * iBuffer, size_t iBufferSize, void* iWriteTo) const {
    return deserializer_.deserialize(iBuffer, iBufferSize, iWriteTo);
  }
 private:
  D deserializer_;
};

using DeserializeStrategy = ProxyVector<DeserializeProxyBase, TClass*>;

}
#endif
