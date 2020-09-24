#if !defined(DataProductRetriever_h)
#define DataProductRetriever_h

#include <string>
#include "DelayedProductRetriever.h"
#include "TaskHolder.h"

class TClass;

namespace cce::tf {
class DataProductRetriever {
 public:
 DataProductRetriever(int iIndex, 
		      void** iAddress,
                       std::string iName,
                       TClass* iClass,
		       DelayedProductRetriever* iDelayed):
  address_(iAddress),
    name_(std::move(iName)),
    class_(iClass),
    delayedReader_(iDelayed),
    index_(iIndex){}

  void** address() const {return address_; }
  size_t size() const { return size_; }
  std::string const& name() const { return name_;}
  TClass* classType() const { return class_;}

  void setAddress(void** iAddress) { address_ = iAddress; }
  void setSize(size_t iSize) { size_ = iSize;}

  void getAsync(TaskHolder iCallback) {
    delayedReader_->getAsync(index_, std::move(iCallback));
  }

  int index() const { return index_;}

 private:
  void** address_;
  size_t size_ = 0;
  std::string name_;
  TClass* class_;
  DelayedProductRetriever* delayedReader_;
  int index_;
};
}
#endif
