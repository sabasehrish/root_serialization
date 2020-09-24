#include "sourceFactoryGenerator.h"

#include "RootSource.h"
#include "RepeatingRootSource.h"
#include "PDSSource.h"
#include "EmptySource.h"

std::function<std::unique_ptr<cce::tf::SourceBase>(unsigned long long)> 
cce::tf::sourceFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<SourceBase>(unsigned long long)> sourceFactory;
  if( iType == "RootSource") {
    std::string fileName( iOptions );
    sourceFactory = [fileName]( unsigned long long iNEvents) {
      return std::make_unique<RootSource>(fileName, iNEvents);
    };
  } else if( iType == "RepeatingRootSource") {
    std::string fileName( iOptions );
    unsigned int nUniqueEvents = 10;
    auto pos = fileName.find(':');
    if(pos != std::string::npos) {
      nUniqueEvents = std::stoul(fileName.substr(pos+1));
      fileName = fileName.substr(0,pos);
    }
    sourceFactory = [fileName, nUniqueEvents](unsigned long long iNEvents) {
      return std::make_unique<RepeatingRootSource>(fileName, nUniqueEvents, iNEvents);
    };
  } else if( iType == "PDSSource") {
    std::string fileName( iOptions );
    sourceFactory = [fileName](unsigned long long iNEvents) {
      return std::make_unique<PDSSource>(fileName, iNEvents);
    };
  } else if( iType == "EmptySource") {
    sourceFactory = [](unsigned long long iNEvents) {
      return std::make_unique<EmptySource>(iNEvents);
    };
  }
  return sourceFactory;
}
