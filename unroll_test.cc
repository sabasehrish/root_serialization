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
#include "test_classes/TestClasses.h"

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

  template<typename T>
  bool compare_containers( T const& iLHS, T const& iRHS) {
    if(std::size(iLHS) != std::size(iRHS)) {
      return false;
    }

    auto it = std::begin(iLHS);
    for(auto const& r: iRHS) {
      if(*it != r) {
        return false;
      }
      std::advance(it,1);
    }
    return true;
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
    std::cout <<"**std::vector<int>**"<<std::endl;
    std::vector<int> iv = {1,2,3};

    auto pV = runTest(iv);
    std::cout <<"size "<<pV.size()<<std::endl;
  }
  {
    std::cout <<"**edm::EventAuxiliary**"<<std::endl;
    edm::EventAuxiliary ev({1,1,1}, "32981", edm::Timestamp{0}, true, edm::EventAuxiliary::PhysicsTrigger,12);
    auto pEv = runTest(ev);
    std::cout <<"eventID "<<pEv.event()<<" "<<pEv.processGUID()<<" bx "<<pEv.bunchCrossing()<<std::endl;
  }

  {
    std::cout <<"**SimpleClass**"<<std::endl;
    cce::tf::test::SimpleClass v(5);
    auto pV = runTest(v);
    if(v.value() != pV.value()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClass**"<<std::endl;
    cce::tf::test::TestClass v("foo", 78.9);
    auto pV = runTest(v);
    if(v.stringValue() != pV.stringValue()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
    if(v.floatValue() != pV.floatValue()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }
  
  {
    std::cout <<"**TestClassWithPointerToSimpleClass**"<<std::endl;
    cce::tf::test::TestClassWithPointerToSimpleClass v(5);
    auto pV = runTest(v);
    if(v.value()->value() != pV.value()->value()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithUniquePointerToSimpleClass**"<<std::endl;
    cce::tf::test::TestClassWithUniquePointerToSimpleClass v(5);
    auto pV = runTest(v);
    if(v.value()->value() != pV.value()->value()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithFloatVector**"<<std::endl;
    cce::tf::test::TestClassWithFloatVector v( {1,2,3,5});
    auto pV = runTest(v);
    if(not compare_containers(v.values(), pV.values())) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithFloatCArray**"<<std::endl;
    cce::tf::test::TestClassWithFloatCArray v(3);
    auto pV = runTest(v);
    if(not compare_containers(v.m_values, pV.m_values)) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithFloatArray**"<<std::endl;
    cce::tf::test::TestClassWithFloatArray v({1,2,3});
    auto pV = runTest(v);
    if(not compare_containers(v.values(), pV.values())) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithFloatDynamicArray**"<<std::endl;
    cce::tf::test::TestClassWithFloatDynamicArray v(3);
    auto pV = runTest(v);
    if(not compare_containers(v.values(), pV.values())) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithSimpleClassVector**"<<std::endl;
    cce::tf::test::TestClassWithSimpleClassVector v( {1,2,3,5});
    auto pV = runTest(v);
    if(not compare_containers(v.values(), pV.values())) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassWithTestClassVector**"<<std::endl;
    cce::tf::test::TestClassWithTestClassVector v( {{"one",1},{"two",2},{"three",3},{"five",5}});
    auto pV = runTest(v);
    if(not compare_containers(v.values(), pV.values())) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**TestClassVectorWithClassWithVector**"<<std::endl;
    cce::tf::test::TestClassVectorWithClassWithVector v( 
                                                        {cce::tf::test::TestClassWithTestClassVector{{{"one",1},{"two",2},{"three",3},{"five",5}}},
                                                            cce::tf::test::TestClassWithTestClassVector{{{"eight",8}, {"one thousand two hundred and 1",1201}}}});
    auto pV = runTest(v);
    if(v.values().size() != pV.values().size()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
    for(int i=0; i<v.values().size(); ++i) {
      if(not compare_containers(v.values()[i].values(), pV.values()[i].values())) {
        std::cout<<"ERROR"<<std::endl;
        return 1;
      }
    }
  }

  {
    std::cout <<"**InheritFromPureAbstractBase**"<<std::endl;
    cce::tf::test::InheritFromPureAbstractBase v(5);
    auto pV = runTest(v);
    if(v.value() != pV.value()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  {
    std::cout <<"**std::vector<InheritFromPureAbstractBase>**"<<std::endl;
    std::vector<cce::tf::test::InheritFromPureAbstractBase> v({1,2,3,5});
    auto pV = runTest(v);
    if(not compare_containers(v,  pV)) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }


  {
    std::cout <<"**InheritFromAbstractInheritingFromBase**"<<std::endl;
    cce::tf::test::InheritFromAbstractInheritingFromBase v(3.14, 5);
    auto pV = runTest(v);
    if(v.iValue() != pV.iValue() or v.fValue() != pV.fValue()) {
      std::cout<<"ERROR"<<std::endl;
      return 1;
    }
  }

  //testNamedClass("edm::Wrapper<edm::Association<vector<reco::DeDxHitInfo> > >");
  for(int i=start; i<argc;++i) {
    std::cout <<"class: "<<argv[i]<<std::endl;
    testNamedClass(argv[i]);
  }
  return 0;
}
