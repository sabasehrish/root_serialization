#if !defined(DataProductRetriever_h)
#define DataProductRetriever_h

#include <string>
class TClass;

class DataProductRetriever {
 public:
  DataProductRetriever(void** iAddress,
                       std::string iName,
                       TClass* iClass):
  address_(iAddress),
    name_(std::move(iName)),
    class_(iClass) {}

  void** address() const {return address_; }
  size_t size() const { return size_; }
  std::string const& name() const { return name_;}
  TClass* classType() const { return class_;}

  void setSize(size_t iSize) { size_ = iSize;}

 private:
  void** address_;
  size_t size_ = 0;
  std::string name_;
  TClass* class_;
};

#endif
