#if !defined(sourceFactoryGenerator_h)
#define sourceFactoryGenerator_h
#include <functional>
#include <string>
#include <string_view>
#include <memory>

#include "SourceBase.h"

std::function<std::unique_ptr<SourceBase>(std::string const&, unsigned long long)> 
sourceFactoryGenerator(std::string_view iType);

#endif
