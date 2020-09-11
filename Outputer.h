#if !defined(Outputer_h)
#define Outputer_h

#include <vector>
#include <string>
#include <iostream>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializerWrapper.h"

#include "SerialTaskQueue.h"

class Outputer :public OutputerBase {
 public:
  void outputAsync(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers, TaskHolder iCallback) const final {
    queue_.push(*iCallback.group(), [this, iEventID, &iSerializers, callback=std::move(iCallback)]() mutable {
	output(iEventID, iSerializers);
	callback.doneWaiting();
      });
  }
  
 private:
  void output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) const {
    using namespace std::string_literals;

    std::cout <<"   run:"s+std::to_string(iEventID.run)+" lumi:"s+std::to_string(iEventID.lumi)+" event:"s+std::to_string(iEventID.event)+"\n"<<std::flush;
    /*
    for(auto& s: iSerializers) {
      std::cout<<"   "s+std::string(s.name())+" size "+std::to_string(s.blob().size())+"\n" <<std::flush;
    }
    */
  }
private:
  mutable SerialTaskQueue queue_;
};

#endif
