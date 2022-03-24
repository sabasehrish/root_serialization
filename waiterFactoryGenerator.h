#if !defined(waiterFactoryGenerator_h)
#define waiterFactoryGenerator_h

#include <functional>
#include <memory>
#include <string_view>
#include "WaiterBase.h"

namespace cce::tf {
std::function<std::unique_ptr<WaiterBase>(size_t)>
waiterFactoryGenerator(std::string_view iType, std::string_view iOptions);
}
#endif
