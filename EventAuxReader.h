#if !defined(EventAuxReader_h)
#define EventAuxReader_h

#include <cassert>

#include "EventIdentifier.h"
#include "art/EventAuxiliary.h"
#include "cms/EventAuxiliary.h"

#include "TBranch.h"
#include "TFile.h"

namespace cce::tf {

  inline EventIdentifier cmsEventID(void** address) {
    assert(address);
    auto aux = *reinterpret_cast<edm::EventAuxiliary**>(address);
    return EventIdentifier{aux->run(), aux->luminosityBlock(), aux->event()};
  }

  inline EventIdentifier artEventID(void** address) {
    assert(address);
    auto aux = *reinterpret_cast<art::EventAuxiliary**>(address);
    return EventIdentifier{aux->run(), aux->subRun(), aux->event()};
  }

  inline std::function<EventIdentifier(void**)> select_reader(TFile& file)
  {
    if (file.GetKey("RootFileDB")) {
      return artEventID;
    }
    return cmsEventID;
  }

  class EventAuxReader {
  public:
    explicit EventAuxReader(TFile& file)
      : func_{select_reader(file)}
    {}

    EventIdentifier doWork(TBranch* eventAuxBranch)
    {
      assert(eventAuxBranch);
      return doWork(reinterpret_cast<void**>(eventAuxBranch->GetAddress()));
    }

    EventIdentifier doWork(void** eventAuxBranch)
    {
      return func_(eventAuxBranch);
    }

  private:
    std::function<EventIdentifier(void**)> func_{};
  };
}
#endif
