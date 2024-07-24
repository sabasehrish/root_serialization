#if !defined(RNTupleOutputerConfig_h)
#define RNTupleOutputerConfig_h

#include <optional>
#include <utility>
#include <Compression.h>


namespace cce::tf {
  struct RNTupleOutputerConfig {
    int verbose_=0;
    int compressionLevel_=9;
    ROOT::RCompressionSetting::EAlgorithm::EValues compressionAlgorithm_= ROOT::RCompressionSetting::EAlgorithm::kUseGlobal;
    std::size_t approxUnzippedPageSize_ = 64 * 1024;
    std::size_t approxZippedClusterSize_ = 50 * 1000 * 1000;
    std::size_t maxUnzippedClusterSize_ = 512 * 1024 * 1024;
    bool hasSmallClusters_=false;
    bool useBufferedWrite_=true;
    bool useTailPageOptimization_=true;
    bool enablePageChecksums_=true;
    bool printEstimateWriteMemoryUsage_=false;
  };


  class ConfigurationParameters;

  std::optional<std::pair<std::string, RNTupleOutputerConfig>> parseRNTupleConfig(ConfigurationParameters const& params);
}


#endif
