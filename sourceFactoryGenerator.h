#if !defined(sourceFactoryGenerator_h)
#define sourceFactoryGenerator_h
#include <functional>
#include <string>
#include <string_view>
#include <memory>

#include "SourceBase.h"

std::function<std::unique_ptr<SourceBase>(unsigned long long)> 
sourceFactoryGenerator(std::string_view iType, std::string_view iOptions);

#endif
