#include "RootOutputerConfig.h"
#include "ConfigurationParameters.h"
#include <iostream>

namespace cce::tf {
  std::optional<std::pair<std::string, RootOutputerConfig>> parseRootConfig(ConfigurationParameters const& params) {
    auto fileName = params.get<std::string>("fileName");
    if(not fileName) {
      std::cout <<"no file name given for Outputer\n";
      return std::nullopt;
    }

    RootOutputerConfig config;
    config.splitLevel_ = params.get<int>("splitLevel",config.splitLevel_);

    config.compressionLevel_ = params.get<int>("compressionLevel", config.compressionLevel_);
    config.compressionAlgorithm_ = params.get<std::string>("compressionAlgorithm", config.compressionAlgorithm_);
    config.basketSize_ = params.get<int>("basketSize", config.basketSize_);
    config.treeMaxVirtualSize_ =  params.get<int>("treeMaxVirtualSize", config.treeMaxVirtualSize_);
    config.autoFlush_ = params.get<int>("autoFlush", config.autoFlush_);
    config.cacheSize_= params.get<int>("cacheSize", config.cacheSize_);
    return std::make_pair(*fileName,config);
  }
}
