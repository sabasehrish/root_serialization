#if !defined(outputerFactoryGenerator_h)
#define outputerFactoryGenerator_h

#include <functional>
#include <memory>
#include <string_view>
#include "OutputerBase.h"

std::function<std::unique_ptr<OutputerBase>(unsigned int)>
outputerFactoryGenerator(std::string_view iType, std::string_view iOptions);

#endif
