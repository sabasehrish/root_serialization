#include "outputerFactoryGenerator.h"
#include "PDSOutputer.h"
#include "SerializeOutputer.h"
#include "DummyOutputer.h"

std::function<std::unique_ptr<cce::tf::OutputerBase>(unsigned int)>
cce::tf::outputerFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<OutputerBase>(unsigned int)> outFactory;
  
  if(iType == "PDSOutputer") {
    std::string outputInfo{iOptions};
    outFactory = [outputInfo](unsigned int nLanes) { return std::make_unique<PDSOutputer>(outputInfo, nLanes);};
  } else if(iType == "SerializeOutputer") {
    bool verbose = false;
    if(not iOptions.empty()) {
      if(iOptions == "verbose") {
        verbose = true;
      }else {
        std::cout <<"Unknown option for SerizeOutputer: "<<iOptions<<std::endl;
        return outFactory;
      }
    }
    outFactory = [verbose](unsigned int nLanes) {return std::make_unique<SerializeOutputer>(nLanes,verbose);};
  } else if(iType == "DummyOutputer") {
    bool useProductReady = false;
    if(not iOptions.empty()) {
      if(iOptions != "useProductReady") {
	std::cout <<"Unknown option for DummyOutputer: "<<iOptions<<std::endl;
	return outFactory;
      }
      useProductReady = true;
    }
    outFactory = [useProductReady](unsigned int) { return std::make_unique<DummyOutputer>(useProductReady);};
  }

  return outFactory;
}
