#include "DummyOutputer.h"
#include "OutputerFactory.h"
#include <iostream>

namespace cce::tf {
namespace {
    class Maker : public OutputerMakerBase {
  public:
    Maker(): OutputerMakerBase("DummyOutputer") {}
    std::unique_ptr<OutputerBase> create(unsigned int iNLanes, ConfigurationParameters const& params) const final {
      bool useProductReady = params.get<bool>("useProductReady",false);
      return std::make_unique<DummyOutputer>(useProductReady);
    }
    };

  Maker s_maker;
}
}
