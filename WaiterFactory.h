#if !defined(WaiterFactory_h)
#define WaiterFactory_h

#include "ComponentFactory.h"
#include "WaiterBase.h"
#include "ConfigurationParameters.h"

namespace cce::tf {
  // arguments are number of lanes followed by number of data products
  using WaiterFactory = ComponentFactory<WaiterBase*(unsigned int, std::size_t , ConfigurationParameters const&)>;
  using WaiterMakerBase = WaiterFactory::CMakerBase;
}

#endif

