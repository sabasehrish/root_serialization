#include "UnrolledSerializer.h"
#include "UnrolledDeserializer.h"

#include "TClass.h"

#include <iostream>
#include <string>

#include "cms/EventAuxiliary.h"

namespace edm::detail {
  static std::string const invalidHash;
  std::string const& InvalidHash() { return invalidHash;}
}

int main() {
  using namespace cce::tf;


  auto cls =TClass::GetClass(typeid(edm::EventAuxiliary));

  if(nullptr == cls) {
    std::cout <<"FAILED TO GET CLASS"<<std::endl;
    return -1;
  }

  UnrolledSerializer us(cls);

  edm::EventAuxiliary ev({1,1,1}, "32981", edm::Timestamp{0}, true);
  auto buffer = us.serialize(&ev);

  std::cout <<"size "<<buffer.size()<<std::endl;

  UnrolledDeserializer ud(cls);
  
  auto pV = ud.deserialize(buffer);
  auto pE = reinterpret_cast<edm::EventAuxiliary*>(pV);

  std::cout <<"eventID "<<pE->event()<<std::endl;
  delete pE;

  return 0;
}
