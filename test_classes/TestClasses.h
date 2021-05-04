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

    bool operator==(SimpleClass const& iOther) const {
      return m_int == iOther.m_int;
    }

    bool operator!=(SimpleClass const& iOther) const {
      return not operator==(iOther);
    }

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

    bool operator==(TestClass const& iOther) const {
      return m_string == iOther.m_string && m_float == iOther.m_float;
    }

    bool operator!=(TestClass const& iOther) const {
      return not operator==(iOther);
    }


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

  class TestClassWithFloatVector {
  public:
    TestClassWithFloatVector() = default;
    TestClassWithFloatVector(std::vector<float> iFloat): m_floats(std::move(iFloat)) {}

    std::vector<float> const& values() const {return m_floats;}
  private:
    std::vector<float> m_floats;
  };

  class TestClassWithFloatCArray {
  public:
    TestClassWithFloatCArray() = default;
    TestClassWithFloatCArray(float iValue) {
      std::fill(std::begin(m_values), std::end(m_values), iValue);
    }
    
    float m_values[3];
  };

  class TestClassWithFloatArray {
  public:
    TestClassWithFloatArray() = default;
    TestClassWithFloatArray(std::array<float,3> iFloat): m_floats(iFloat) {}

    std::array<float,3> const& values() const {return m_floats;}
  private:
    std::array<float,3> m_floats;
  };

  class TestClassWithFloatDynamicArray {
  public:
    TestClassWithFloatDynamicArray(): m_size(0), m_floats(nullptr) {}
    TestClassWithFloatDynamicArray(float iValue) {
      m_size = 3;
      m_floats = new float[m_size];
      std::fill(m_floats, m_floats+m_size, iValue);
    }
    ~TestClassWithFloatDynamicArray() {
      delete [] m_floats;
    }
    int size() const {return m_size;}
    float const* begin() const {return m_floats;}
    float const* end() const { return m_floats+m_size;}

    std::vector<float> values() const {return std::vector<float>(begin(), end());}

  private:
    int m_size; ///<
    float* m_floats; ///<[m_size]
  };


  template<typename T>
  class TestClassWithVector {
  public:
    TestClassWithVector() = default;
    TestClassWithVector(std::vector<T> iValues): m_values(std::move(iValues)) {}

    std::vector<T> const& values() const {return m_values;}
  private:
    std::vector<T> m_values;
  };

  using TestClassWithSimpleClassVector = TestClassWithVector<SimpleClass>;
  using TestClassWithTestClassVector = TestClassWithVector<TestClass>;
  using TestClassVectorWithClassWithVector = TestClassWithVector<TestClassWithVector<TestClass>>;


}
#endif
