#if !defined(RootOutputerConfig_h)
#define RootOutputerConfig_h

#include <string>
#include <optional>
#include <utility>

namespace cce::tf {
  struct RootOutputerConfig {
    int splitLevel_=99;
    int compressionLevel_=9;
    std::string compressionAlgorithm_="";
    int basketSize_=16384;
    int treeMaxVirtualSize_=-1;
    int autoFlush_=-1;
    int cacheSize_=0;
  };

  template<typename T>
  inline T outputerConfig(RootOutputerConfig const& iConfig) {
    T config;
    config.splitLevel_ = iConfig.splitLevel_;
    config.compressionLevel_ = iConfig.compressionLevel_;
    config.compressionAlgorithm_ = iConfig.compressionAlgorithm_;
    config.basketSize_ = iConfig.basketSize_;
    config.treeMaxVirtualSize_ = iConfig.treeMaxVirtualSize_;
    config.autoFlush_ = iConfig.autoFlush_;
    return config;
  }

  class ConfigurationParameters;

  std::optional<std::pair<std::string, RootOutputerConfig>> parseRootConfig(ConfigurationParameters const& params);
}


#endif
