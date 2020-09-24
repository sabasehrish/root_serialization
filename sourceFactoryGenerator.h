#if !defined(sourceFactoryGenerator_h)
#define sourceFactoryGenerator_h
#include <functional>
#include <string>
#include <string_view>
#include <memory>

#include "SharedSourceBase.h"

namespace cce::tf {
  std::function<std::unique_ptr<SharedSourceBase>(unsigned int, unsigned long long)> 
    sourceFactoryGenerator(std::string_view iType, std::string_view iOptions);
}
#endif
