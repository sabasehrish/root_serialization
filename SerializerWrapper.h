#if !defined(SerializerWrapper_h)
#define SerializerWrapper_h

#include <vector>
#include <chrono>
#include "TClass.h"

#include "tbb/task_group.h"
#include "Serializer.h"
#include "TaskHolder.h"


class SerializerWrapper {
public:
 SerializerWrapper(std::string_view iName,  TClass* tClass):
  name_{iName}, class_(tClass), serializer_{},
  accumulatedTime_{std::chrono::microseconds::zero()} {}

  void doWorkAsync(tbb::task_group& iGroup, void** iAddress, TaskHolder iCallback) {
    iGroup.run([this, iAddress, callback=std::move(iCallback)] () {
	{
	  auto start = std::chrono::high_resolution_clock::now();
	  blob_ = serializer_.serialize(*iAddress, class_);
	  accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
	}
	const_cast<TaskHolder&>(callback).doneWaiting();
      });
  }
  std::vector<char> const& blob() const {return blob_;}

  std::string_view  name() const {return name_;}
  char const* className() const { return class_->GetName(); }
  std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}
private:
  std::vector<char> blob_;
  std::string_view name_;
  TClass* class_;
  Serializer serializer_;
  std::chrono::microseconds accumulatedTime_;
};
#endif
