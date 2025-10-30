#if !defined(RNTupleOutputerConfig_h)
#define RNTupleOutputerConfig_h

#include <optional>
#include <utility>
#include <Compression.h>
#include <ROOT/RNTupleWriter.hxx>

namespace cce::tf {
  struct RNTupleOutputerConfig {
    RNTupleOutputerConfig() {
      ROOT::RNTupleWriteOptions options;
      maxUnzippedPageSize_ = options.GetMaxUnzippedPageSize();
      approxZippedClusterSize_ = options.GetApproxZippedClusterSize();
      maxUnzippedClusterSize_ = options.GetMaxUnzippedClusterSize();
      pageBufferBudget_ = options.GetPageBufferBudget();
    }
    int verbose_=0;
    int compressionLevel_=9;
    ROOT::RCompressionSetting::EAlgorithm::EValues compressionAlgorithm_= ROOT::RCompressionSetting::EAlgorithm::kUseGlobal;
    std::size_t maxUnzippedPageSize_;
    std::size_t approxZippedClusterSize_;
    std::size_t maxUnzippedClusterSize_;
    std::size_t pageBufferBudget_;
    bool useBufferedWrite_=true;
    bool useDirectIO_=true;
    bool enablePageChecksums_=true;
    bool printEstimateWriteMemoryUsage_=false;
  };


  class ConfigurationParameters;

  std::optional<std::pair<std::string, RNTupleOutputerConfig>> parseRNTupleConfig(ConfigurationParameters const& params);
  ROOT::RNTupleWriteOptions writeOptionsFrom(RNTupleOutputerConfig const&);
}


#endif
