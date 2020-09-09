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
  std::string const& name() const { return name_;}
  TClass* classType() const { return class_;}

 private:
  void** address_;
  std::string name_;
  TClass* class_;
};

#endif
