#if !defined(SerializerWrapper_h)
#define SerializerWrapper_h

#include <vector>
#include "TClass.h"

#include "tbb/task_group.h"
#include "Serializer.h"
#include "TaskHolder.h"


class SerializerWrapper {
public:
  SerializerWrapper(char const* iName, void** iAddress, TClass* tClass):
    name_{iName}, address_(iAddress), class_(tClass), serializer_{} {}

  void doWorkAsync(tbb::task_group& iGroup, TaskHolder iCallback) {
    iGroup.run([this, callback=std::move(iCallback)] () {
	blob_ = serializer_.serialize(*address_, class_);
	const_cast<TaskHolder&>(callback).doneWaiting();
      });
  }
  std::vector<char> const& blob() const {return blob_;}

  char const* name() const {return name_;}
private:
  std::vector<char> blob_;
  char const* name_;
  void** address_;
  TClass* class_;
  Serializer serializer_;
};
#endif
