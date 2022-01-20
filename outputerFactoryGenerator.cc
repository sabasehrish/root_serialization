#include "outputerFactoryGenerator.h"
#include "OutputerFactory.h"
#include "RootOutputer.h"
#include "SerializeOutputer.h"
#include "DummyOutputer.h"
#include "TextDumpOutputer.h"
#include "TBufferMergerRootOutputer.h"
#include "TestProductsOutputer.h"
#include "configKeyValuePairs.h"

namespace {
  struct RootConfig {
    int splitLevel_=99;
    int compressionLevel_=9;
    std::string compressionAlgorithm_="";
    int basketSize_=16384;
    int treeMaxVirtualSize_=-1;
    int autoFlush_=-1;
  };

  template<typename T>
  T outputerConfig(RootConfig const& iConfig) {
    T config;
    config.splitLevel_ = iConfig.splitLevel_;
    config.compressionLevel_ = iConfig.compressionLevel_;
    config.compressionAlgorithm_ = iConfig.compressionAlgorithm_;
    config.basketSize_ = iConfig.basketSize_;
    config.treeMaxVirtualSize_ = iConfig.treeMaxVirtualSize_;
    config.autoFlush_ = iConfig.autoFlush_;
    return config;
  }
  std::pair<bool, std::string> preParseTBufferMerger(std::string_view iOptions) {
    std::string remainingOptions{iOptions};
    auto pos = remainingOptions.find("concurrentWrite=");
    bool set = true;
    if(pos != std::string::npos) {
      set = (remainingOptions[pos+16] == 'y' or remainingOptions[pos+16] == 'Y') ;
      auto size = 17;
      if(remainingOptions.size() > pos+17) {
        auto npos = remainingOptions.find(pos+17,':');
        if(npos != std::string::npos) {
          size = npos - pos;
        }
      }
      remainingOptions.replace(pos,size,"");
    }
    return std::make_pair(set, remainingOptions);
  }

  std::optional<std::pair<std::string, RootConfig>> parseRootConfig(std::string_view iOptions) {
    std::string fileName{iOptions};
    RootConfig config;
    auto pos = fileName.find(':');
    if(pos != std::string::npos) {
      auto remainingOptions = fileName.substr(pos+1);
      fileName = fileName.substr(0,pos);

      auto keyValues = cce::tf::configKeyValuePairs(remainingOptions);
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
      itFound = keyValues.find("autoFlush");
      if(itFound != keyValues.end()) {
	config.autoFlush_ = std::stoul(itFound->second);
	++foundOptions;
      }
      if(foundOptions != keyValues.size()) {
	std::cout <<"Unknown options for RootOutputer "<<remainingOptions<<std::endl;
	for(auto const& kv: keyValues) {
	  std::cout <<kv.first<<" "<<kv.second<<std::endl;
	}
	return std::nullopt;
      }
    }
    return std::make_pair(fileName,config);
  }

  /*
  struct HDFConfig {
    int maxBatchSize=1;
  };

  std::optional<std::pair<std::string, HDFConfig>> parseHDFConfig(std::string_view iOptions) {
    std::string fileName{iOptions};
    HDFConfig config;
    auto pos = fileName.find(':');
    if(pos != std::string::npos) {
      auto remainingOptions = fileName.substr(pos+1);
      fileName = fileName.substr(0,pos);

      auto keyValues = cce::tf::configKeyValuePairs(remainingOptions);
      int foundOptions = 0;
      auto itFound = keyValues.find("batchSize");
      if(itFound != keyValues.end()) {
	config.maxBatchSize = std::stoul(itFound->second);
	++foundOptions;
      }
      if(foundOptions != keyValues.size()) {
	std::cout <<"Unknown options for HDFOutputer "<<remainingOptions<<std::endl;
	for(auto const& kv: keyValues) {
	  std::cout <<kv.first<<" "<<kv.second<<std::endl;
	}
	return std::nullopt;
      }
    }
    return std::make_pair(fileName,config);
  }
  */

  struct TextDumpConfig {
    bool perEvent=true;
    bool summary=false;
  };

  std::optional<TextDumpConfig> parseTextDumpConfig(std::string_view iOptions) {
    TextDumpConfig conf;
    auto keyValues = cce::tf::configKeyValuePairs(iOptions);
    auto itFound = keyValues.find("perEvent");
    int count = 0;
    if(itFound != keyValues.end()) {
      if(not itFound->second.empty() and itFound->second[0]!='t') {
        conf.perEvent = false;
      }
      ++count;
    }
    itFound = keyValues.find("summary");
    if(itFound != keyValues.end()) {
      if(itFound->second.empty() or itFound->second[0]=='t') {
        conf.summary = true;
      }
      ++count;
    }
    if(keyValues.size() != count) {
      std::cout <<"unknown options passed to TextDumpOutputer, only use 'perEvent' and 'summary'"<<std::endl;
      return std::nullopt;
    }
    return conf;
  }
}

std::function<std::unique_ptr<cce::tf::OutputerBase>(unsigned int)>
cce::tf::outputerFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<OutputerBase>(unsigned int)> outFactory;
  
  if(iType == "PDSOutputer") {

    std::string fileName{iOptions};
    auto pos = fileName.find(':');
    std::map<std::string, std::string> keyValues;
    if(pos != std::string::npos) {
      auto remainingOptions = fileName.substr(pos+1);
      fileName = fileName.substr(0,pos);

      keyValues = cce::tf::configKeyValuePairs(remainingOptions);
    }
    keyValues["fileName"] = fileName;

    outFactory = [keyValues](unsigned int nLanes) { return OutputerFactory::get()->create("PDSOutputer", nLanes, keyValues); };
    return outFactory;
  } else if(iType == "RootOutputer") {
    auto result = parseRootConfig(iOptions);
    if(not result) {
      return outFactory;
    }
    auto fileName = result->first;
    auto config = outputerConfig<RootOutputer::Config>(result->second);
    
    outFactory = [fileName,config](unsigned int nLanes) { return std::make_unique<RootOutputer>(fileName, nLanes, config);};
  } else if(iType == "TBufferMergerRootOutputer") {
    auto preParse = preParseTBufferMerger(iOptions);
    auto result = parseRootConfig(preParse.second);
    if(not result) {
      return outFactory;
    }
    auto fileName = result->first;
    auto config = outputerConfig<TBufferMergerRootOutputer::Config>(result->second);
    config.concurrentWrite = preParse.first;

    outFactory = [fileName,config](unsigned int nLanes) { return std::make_unique<TBufferMergerRootOutputer>(fileName, nLanes, config);};
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
  } else if(iType == "HDFOutputer") {

    std::string fileName{iOptions};
    auto pos = fileName.find(':');
    std::map<std::string, std::string> keyValues;
    if(pos != std::string::npos) {
      auto remainingOptions = fileName.substr(pos+1);
      fileName = fileName.substr(0,pos);

      keyValues = cce::tf::configKeyValuePairs(remainingOptions);
    }
    keyValues["fileName"] = fileName;

    outFactory = [keyValues](unsigned int nLanes) { return OutputerFactory::get()->create("HDFOutputer", nLanes, keyValues); };
    return outFactory;
  } else if(iType == "TextDumpOutputer") {
    auto result = parseTextDumpConfig(iOptions);
    if(not result) {
      return outFactory;
    }
    outFactory = [config = *result](unsigned int) { return std::make_unique<TextDumpOutputer>(config.perEvent, config.summary);};
  } else if(iType == "TestProductsOutputer") {
    outFactory = [](unsigned int iNLanes) { return std::make_unique<TestProductsOutputer>(iNLanes);};
  }
  return outFactory;
}
