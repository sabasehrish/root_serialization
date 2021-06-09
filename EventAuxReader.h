#if !defined(EventAuxReader_h)
#define EventAuxReader_h

#include <cassert>

#include "EventIdentifier.h"
#include "art/EventAuxiliary.h"
#include "cms/EventAuxiliary.h"

#include "TFile.h"
class TBranch;

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
    EventAuxReader() = default;

    explicit EventAuxReader(TFile& file)
      : func_{select_reader(file)}
    {}

    EventAuxReader& bindToBranch(void** branchToRead) noexcept
    {
      boundBranch_ = reinterpret_cast<void**>(branchToRead);
      return *this;
    }

    EventAuxReader& bindToBranch(TBranch* branchToRead) noexcept
    {
      return bindToBranch(reinterpret_cast<void**>(branchToRead));
    }

    EventIdentifier doWork()
    {
      if(boundBranch_ == nullptr) {
        return EventIdentifier{0,0,0};
      }
      assert(func_);
      return func_(boundBranch_);
    }

  private:
    std::function<EventIdentifier(void**)> func_{};
    void** boundBranch_{nullptr};
  };
}
#endif
