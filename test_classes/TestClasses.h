#if !defined(cce_tf_TestClasses_h)
#define cce_tf_TestClasses_h

#include <vector>
#include <string>
#include <memory>

namespace cce::tf::test {

  class SimpleClass {
  public:
    SimpleClass() : m_int(1) {}
    SimpleClass(int iValue) : m_int(iValue) {}
    
    int value() const {return m_int;}

  private:
    int m_int;
  };

  class EmptyBase {
  public:
    virtual ~EmptyBase() = default;
  };

  class TestClass : public EmptyBase {
  public:
    TestClass(): m_string("default"), m_float(3.14) {}

  TestClass(std::string iString, float iFloat): m_string(std::move(iString)), m_float(iFloat) {}

    std::string const& stringValue() const {return m_string;}
    float floatValue() { return m_float;}

  private:

    std::string m_string;
    float m_float;
  };

  class TestClassWithPointerToSimpleClass {
  public:
    TestClassWithPointerToSimpleClass(): m_ptr(nullptr) {}
    TestClassWithPointerToSimpleClass(int iValue) : m_ptr(new SimpleClass(iValue)) {}

    TestClassWithPointerToSimpleClass(TestClassWithPointerToSimpleClass const&) = delete;
    TestClassWithPointerToSimpleClass& operator=(TestClassWithPointerToSimpleClass const&) = delete;

    TestClassWithPointerToSimpleClass(TestClassWithPointerToSimpleClass&& iOther) : m_ptr(nullptr) {
      std::swap(m_ptr, iOther.m_ptr);
    }
    TestClassWithPointerToSimpleClass& operator=(TestClassWithPointerToSimpleClass&& iOther) {
      TestClassWithPointerToSimpleClass tmp(std::move(iOther));
      std::swap(m_ptr, tmp.m_ptr);
      return *this;
    }

    ~TestClassWithPointerToSimpleClass() {
      delete m_ptr;
    }

    SimpleClass const* value() const { return m_ptr;}

    
  private:
    SimpleClass* m_ptr;
  };

  class TestClassWithUniquePointerToSimpleClass {
  public:
    TestClassWithUniquePointerToSimpleClass(): m_ptr(nullptr) {}
    TestClassWithUniquePointerToSimpleClass(int iValue) : m_ptr(std::make_unique<SimpleClass>(iValue)) {}
    
    TestClassWithUniquePointerToSimpleClass(TestClassWithUniquePointerToSimpleClass const&) = delete;
    TestClassWithUniquePointerToSimpleClass& operator=(TestClassWithUniquePointerToSimpleClass const&) = delete;

    TestClassWithUniquePointerToSimpleClass(TestClassWithUniquePointerToSimpleClass&& iOther) = default;
    TestClassWithUniquePointerToSimpleClass& operator=(TestClassWithUniquePointerToSimpleClass&& iOther) = default;

    SimpleClass const* value() const { return m_ptr.get();}
    
  private:
    std::unique_ptr<SimpleClass> m_ptr;
  };

  class TestClassWithIntPointer {
  public:
    TestClassWithIntPointer(): m_ptr(nullptr) {}
    TestClassWithIntPointer(int iValue) : m_ptr(new int(iValue)) {}

    TestClassWithIntPointer(TestClassWithIntPointer const&) = delete;
    TestClassWithIntPointer& operator=(TestClassWithIntPointer const&) = delete;

    TestClassWithIntPointer(TestClassWithIntPointer&& iOther) : m_ptr(nullptr) {
      std::swap(m_ptr, iOther.m_ptr);
    }
    TestClassWithIntPointer& operator=(TestClassWithIntPointer&& iOther) {
      TestClassWithIntPointer tmp(std::move(iOther));
      std::swap(m_ptr, tmp.m_ptr);
      return *this;
    }

    ~TestClassWithIntPointer() {
      delete m_ptr;
    }

    int const* value() const { return m_ptr;}

    
  private:
    int* m_ptr;
  };

  class TestClassWithIntUniquePointer {
  public:
    TestClassWithIntUniquePointer(): m_ptr(nullptr) {}
    TestClassWithIntUniquePointer(int iValue) : m_ptr(std::make_unique<int>(iValue)) {}
    
    TestClassWithIntUniquePointer(TestClassWithIntUniquePointer const&) = delete;
    TestClassWithIntUniquePointer& operator=(TestClassWithIntUniquePointer const&) = delete;

    TestClassWithIntUniquePointer(TestClassWithIntUniquePointer&& iOther) = default;
    TestClassWithIntUniquePointer& operator=(TestClassWithIntUniquePointer&& iOther) = default;

    int const* value() const { return m_ptr.get();}
    
  private:
    std::unique_ptr<int> m_ptr;
  };

}
#endif
