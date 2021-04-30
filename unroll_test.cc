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
  T runTest(T const& iObject) {
  using namespace cce::tf;

  auto cls =TClass::GetClass(typeid(T));
  //std::cout <<cls<<" TClonesArray::Class "<<TClonesArray::Class()<<std::endl;


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
  
  T newObj;
  ud.deserialize(buffer, &newObj);

  return newObj;
  }

  void testNamedClass(const char* iName) {
    TClass* cls = TClass::GetClass(iName);

    if(nullptr == cls) {
      std::cout <<"FAILED TO GET CLASS "<<iName<<std::endl;
      abort();
    }

    
    auto obj = cls->New();
    
    cce::tf::UnrolledSerializer us(cls);
    auto b = us.serialize(obj);
    std::cout <<"unrolled size "<<b.size()<<std::endl;
    
    {
      cce::tf::Serializer s;
      auto b = s.serialize(obj, cls);
      std::cout <<"standard size "<<b.size()<<std::endl;
    }

  }
}


int main(int argc, char** argv) {
  int start=1;

  const std::string debugFlag("-g");
  if(argc > 1) {
    if (debugFlag == argv[1] ) {
      gDebug = 3;
      start +=1;
    }
  }

  {
    std::vector<int> iv = {1,2,3};

    auto pV = runTest(iv);
    std::cout <<"size "<<pV.size()<<std::endl;
  }
  {
    edm::EventAuxiliary ev({1,1,1}, "32981", edm::Timestamp{0}, true, edm::EventAuxiliary::PhysicsTrigger,12);
    auto pEv = runTest(ev);
    std::cout <<"eventID "<<pEv.event()<<" "<<pEv.processGUID()<<" bx "<<pEv.bunchCrossing()<<std::endl;
  }

  //testNamedClass("edm::Wrapper<edm::Association<vector<reco::DeDxHitInfo> > >");
  for(int i=start; i<argc;++i) {
    std::cout <<"class: "<<argv[i]<<std::endl;
    testNamedClass(argv[i]);
  }
  return 0;
}
