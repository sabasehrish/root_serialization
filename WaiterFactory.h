#if !defined(WaiterFactory_h)
#define WaiterFactory_h

#include "ComponentFactory.h"
#include "WaiterBase.h"
#include "ConfigurationParameters.h"

namespace cce::tf {
  using WaiterFactory = ComponentFactory<WaiterBase*(std::size_t , ConfigurationParameters const&)>;
  using WaiterMakerBase = WaiterFactory::CMakerBase;
}

#endif

