#if !defined(OutputerFactory_h)
#define OutputerFactory_h

#include "ComponentFactory.h"
#include "OutputerBase.h"
#include <map>


namespace cce::tf {
  using OutputerFactory = ComponentFactory<OutputerBase*(unsigned int, std::map<std::string,std::string> const&)>;
  using OutputerMakerBase = OutputerFactory::CMakerBase;
}

#endif

