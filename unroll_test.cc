#include "UnrolledSerializer.h"
#include "UnrolledDeserializer.h"
#include "Serializer.h"

#include "TClass.h"

#include <iostream>
#include <string>
#include <memory>

#include "cms/EventAuxiliary.h"

namespace edm::detail {
  static std::string const invalidHash;
  std::string const& InvalidHash() { return invalidHash;}
}


namespace {
  template<typename T>
  std::unique_ptr<T> runTest(T const& iObject) {
  using namespace cce::tf;

  auto cls =TClass::GetClass(typeid(T));

  if(nullptr == cls) {
    std::cout <<"FAILED TO GET CLASS"<<std::endl;
    abort();
  }

  UnrolledSerializer us(cls);

  auto buffer = us.serialize(&iObject);

  std::cout <<"unrolled size "<<buffer.size()<<std::endl;

  {
    Serializer s;
    auto b = s.serialize(&iObject, cls);
    std::cout <<"standard size "<<b.size()<<std::endl;
  }

  UnrolledDeserializer ud(cls);
  
  auto pV = ud.deserialize(buffer);
  auto pE = reinterpret_cast<T*>(pV);

  return std::unique_ptr<T>(pE);
  }
}

int main() {

  edm::EventAuxiliary ev({1,1,1}, "32981", edm::Timestamp{0}, true);
  auto pEv = runTest(ev);
  std::cout <<"eventID "<<pEv->event()<<std::endl;

  return 0;
}
