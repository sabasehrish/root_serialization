#if !defined(Outputer_h)
#define Outputer_h

#include <mutex>
#include <vector>
#include <string>
#include <iostream>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializerWrapper.h"

class Outputer :public OutputerBase {
public:
  void output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) const final{
    using namespace std::string_literals;

    std::unique_lock lock{mutex_};
    std::cout <<"   run:"s+std::to_string(iEventID.run)+" lumi:"s+std::to_string(iEventID.lumi)+" event:"s+std::to_string(iEventID.event)+"\n"<<std::flush;
    /*
    for(auto& s: iSerializers) {
      std::cout<<"   "s+s.name()+" size "+std::to_string(s.blob().size())+"\n" <<std::flush;
    }
    */
  }
private:
  mutable std::mutex mutex_;
};

#endif
