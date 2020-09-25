#include "sourceFactoryGenerator.h"

#include "RootSource.h"
#include "RepeatingRootSource.h"
#include "PDSSource.h"
#include "EmptySource.h"
#include "ReplicatedSharedSource.h"

std::function<std::unique_ptr<cce::tf::SharedSourceBase>(unsigned int, unsigned long long)> 
cce::tf::sourceFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<SharedSourceBase>(unsigned int, unsigned long long)> sourceFactory;
  if( iType == "RootSource") {
    std::string fileName( iOptions );
    sourceFactory = [fileName](unsigned int iNLanes, unsigned long long iNEvents) {
      return std::make_unique<ReplicatedSharedSource<RootSource>>(iNLanes, iNEvents, fileName);
    };
  } else if( iType == "RepeatingRootSource") {
    std::string fileName( iOptions );
    unsigned int nUniqueEvents = 10;
    auto pos = fileName.find(':');
    if(pos != std::string::npos) {
      nUniqueEvents = std::stoul(fileName.substr(pos+1));
      fileName = fileName.substr(0,pos);
    }
    sourceFactory = [fileName, nUniqueEvents](unsigned int iNLanes, unsigned long long iNEvents) {
      return std::make_unique<RepeatingRootSource>(fileName, nUniqueEvents, iNLanes, iNEvents);
    };
  } else if( iType == "PDSSource") {
    std::string fileName( iOptions );
    sourceFactory = [fileName](unsigned int iNLanes, unsigned long long iNEvents) {
      return std::make_unique<ReplicatedSharedSource<PDSSource>>(iNLanes, iNEvents, fileName);
    };
  } else if( iType == "EmptySource") {
    sourceFactory = [](unsigned int iNLanes, unsigned long long iNEvents) {
      return std::make_unique<EmptySource>(iNEvents);
      };
    } 
  return sourceFactory;
}
