#include "sourceFactoryGenerator.h"

#include "RootSource.h"
#include "PDSSource.h"
#include "EmptySource.h"

std::function<std::unique_ptr<SourceBase>(std::string const&, unsigned long long)> 
sourceFactoryGenerator(std::string_view iType) {
  std::function<std::unique_ptr<SourceBase>(std::string const&, unsigned long long)> sourceFactory;
  if( iType == "RootSource") {
    sourceFactory = [](std::string const& iName, unsigned long long iNEvents) {
      return std::make_unique<RootSource>(iName, iNEvents);
    };
  } else if( iType == "PDSSource") {
    sourceFactory = [](std::string const& iName, unsigned long long iNEvents) {
      return std::make_unique<PDSSource>(iName, iNEvents);
    };
  } else if( iType == "EmptySource") {
    sourceFactory = [](std::string const& iName, unsigned long long iNEvents) {
      return std::make_unique<EmptySource>(iNEvents);
    };
  }
  return sourceFactory;
}
