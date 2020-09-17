#include "outputerFactoryGenerator.h"
#include "PDSOutputer.h"
#include "SerializeOutputer.h"
#include "DummyOutputer.h"

std::function<std::unique_ptr<OutputerBase>(unsigned int)>
outputerFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<OutputerBase>(unsigned int)> outFactory;
  
  if(iType == "PDSOutputer") {
    std::string outputInfo{iOptions};
    outFactory = [outputInfo](unsigned int nLanes) { return std::make_unique<PDSOutputer>(outputInfo, nLanes);};
  } else if(iType == "SerializeOutputer") {
    outFactory = [](unsigned int nLanes) {return std::make_unique<SerializeOutputer>(nLanes);};
  } else if(iType == "DummyOutputer") {
    outFactory = [](unsigned int) { return std::make_unique<DummyOutputer>();};
  }

  return outFactory;
}
