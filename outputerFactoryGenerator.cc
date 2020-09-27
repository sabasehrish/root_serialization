#include "outputerFactoryGenerator.h"
#include "PDSOutputer.h"
#include "RootOutputer.h"
#include "SerializeOutputer.h"
#include "DummyOutputer.h"
#include "configKeyValuePairs.h"

std::function<std::unique_ptr<cce::tf::OutputerBase>(unsigned int)>
cce::tf::outputerFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<OutputerBase>(unsigned int)> outFactory;
  
  if(iType == "PDSOutputer") {
    std::string outputInfo{iOptions};
    outFactory = [outputInfo](unsigned int nLanes) { return std::make_unique<PDSOutputer>(outputInfo, nLanes);};
  } else if(iType == "RootOutputer") {
    std::string fileName{iOptions};
    RootOutputer::Config config;
    auto pos = fileName.find(':');
    if(pos != std::string::npos) {
      auto remainingOptions = fileName.substr(pos+1);
      fileName = fileName.substr(0,pos);

      auto keyValues = configKeyValuePairs(remainingOptions);
      int foundOptions = 0;
      auto itFound = keyValues.find("splitLevel");
      if(itFound != keyValues.end()) {
	config.splitLevel_ = std::stoul(itFound->second);
	++foundOptions;
      }
      itFound = keyValues.find("compressionLevel");
      if(itFound != keyValues.end()) {
	config.compressionLevel_ = std::stoul(itFound->second);
	++foundOptions;
      }
      itFound = keyValues.find("compressionAlgorithm");
      if(itFound != keyValues.end()) {
	config.compressionAlgorithm_ = itFound->second;
	++foundOptions;
      }
      itFound = keyValues.find("basketSize");
      if(itFound != keyValues.end()) {
	config.basketSize_ = std::stoul(itFound->second);
	++foundOptions;
      }
      itFound = keyValues.find("treeMaxVirtualSize");
      if(itFound != keyValues.end()) {
	config.treeMaxVirtualSize_ = std::stoul(itFound->second);
	++foundOptions;
      }
      itFound = keyValues.find("useIMT");
      if(itFound != keyValues.end()) {
	config.useIMT_ = true;
	++foundOptions;
      }
      if(foundOptions != keyValues.size()) {
	std::cout <<"Unknown options for RootOutputer "<<remainingOptions<<std::endl;
	for(auto const& kv: keyValues) {
	  std::cout <<kv.first<<" "<<kv.second<<std::endl;
	}
	return outFactory;
      }
    }
    outFactory = [fileName,config](unsigned int nLanes) { return std::make_unique<RootOutputer>(fileName, nLanes, config);};
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
