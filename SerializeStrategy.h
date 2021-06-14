#if !defined(SerializeStrategy_h)
#define SerializeStrategy_h


#include <vector>
#include <chrono>
#include "TClass.h"

#include "tbb/task_group.h"

#include "TaskHolder.h"
#include "ProxyVector.h"

namespace cce::tf {
class SerializeProxyBase {
 public:
 SerializeProxyBase() = default;

 virtual ~SerializeProxyBase();

 virtual void doWorkAsync(tbb::task_group& iGroup, void** iAddress, TaskHolder iCallback) = 0;
 virtual std::vector<char> const& blob() const = 0;

 virtual std::string_view  name() const = 0;
 virtual char const* className() const = 0;
 virtual std::chrono::microseconds accumulatedTime() const = 0;
};


template<typename WRAPPER>
class SerializeProxy final : public SerializeProxyBase {
 public:
 SerializeProxy(std::string_view iName,  TClass* tClass):
  wrapper_{iName, tClass} {}

  void doWorkAsync(tbb::task_group& iGroup, void** iAddress, TaskHolder iCallback) {
    wrapper_.doWorkAsync(iGroup, iAddress, iCallback);
  }
  std::vector<char> const& blob() const { return wrapper_.blob(); }

  std::string_view  name() const { return wrapper_.name();}
  char const* className() const { return wrapper_.className();}
  std::chrono::microseconds accumulatedTime() const {return wrapper_.accumulatedTime();}
 private:
  WRAPPER wrapper_;
};
 

 using SerializeStrategy = ProxyVector<SerializeProxyBase, std::string_view, TClass*>;

}
#endif
