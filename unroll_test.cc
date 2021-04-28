#include "UnrolledSerializer.h"
#include "UnrolledDeserializer.h"
#include "Serializer.h"

#include "TClass.h"
#include "TClonesArray.h"

#include <iostream>
#include <string>
#include <memory>
#include <vector>

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
  std::cout <<cls<<" TClonesArray::Class "<<TClonesArray::Class()<<std::endl;


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
  //gDebug = 10;

  {
    std::vector<int> iv = {1,2,3};

    auto pV = runTest(iv);
    std::cout <<"size "<<pV->size()<<std::endl;
  }
  {
    edm::EventAuxiliary ev({1,1,1}, "32981", edm::Timestamp{0}, true, edm::EventAuxiliary::PhysicsTrigger,12);
    auto pEv = runTest(ev);
    std::cout <<"eventID "<<pEv->event()<<" "<<pEv->processGUID()<<" bx "<<ev.bunchCrossing()<<std::endl;
  }

  return 0;
}
