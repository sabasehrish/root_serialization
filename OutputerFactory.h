#if !defined(OutputerFactory_h)
#define OutputerFactory_h

#include "ComponentFactory.h"
#include "OutputerBase.h"
#include "ConfigurationParameters.h"
#include <map>


namespace cce::tf {
  using OutputerFactory = ComponentFactory<OutputerBase*(unsigned int, ConfigurationParameters const&, int)>;
  using OutputerMakerBase = OutputerFactory::CMakerBase;
}

#endif

