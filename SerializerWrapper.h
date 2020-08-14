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
  SerializerWrapper(char const* iName, void** iAddress, TClass* tClass):
  name_{iName}, address_(iAddress), class_(tClass), serializer_{},
  accumulatedTime_{std::chrono::microseconds::zero()} {}

  void doWorkAsync(tbb::task_group& iGroup, TaskHolder iCallback) {
    iGroup.run([this, callback=std::move(iCallback)] () {
	{
	  auto start = std::chrono::high_resolution_clock::now();
	  blob_ = serializer_.serialize(*address_, class_);
	  accumulatedTime_ += std::chrono::duration_cast<decltype(accumulatedTime_)>(std::chrono::high_resolution_clock::now() - start);
	}
	const_cast<TaskHolder&>(callback).doneWaiting();
      });
  }
  std::vector<char> const& blob() const {return blob_;}

  char const* name() const {return name_;}
  std::chrono::microseconds accumulatedTime() const { return accumulatedTime_;}
private:
  std::vector<char> blob_;
  char const* name_;
  void** address_;
  TClass* class_;
  Serializer serializer_;
  std::chrono::microseconds accumulatedTime_;
};
#endif
