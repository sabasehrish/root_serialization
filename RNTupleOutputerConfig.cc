#include "RNTupleOutputerConfig.h"

#include "ConfigurationParameters.h"
#include <iostream>

namespace cce::tf {
  std::optional<std::pair<std::string, RNTupleOutputerConfig>> parseRNTupleConfig(ConfigurationParameters const& params) {
    auto fileName = params.get<std::string>("fileName");
    if(not fileName) {
      std::cout <<"no file name given for Outputer\n";
      return std::nullopt;
    }

    RNTupleOutputerConfig config;
    config.compressionLevel_ = params.get<int>("compressionLevel", config.compressionLevel_);
    auto compressionAlgo = params.get<std::string>("compressionAlgorithm", "");
    if (compressionAlgo == "") {
    } else if(compressionAlgo == "lzma") {
      config.compressionAlgorithm_ = ROOT::RCompressionSetting::EAlgorithm::kLZMA;
    } else if(compressionAlgo == "zlib") {
      config.compressionAlgorithm_ = ROOT::RCompressionSetting::EAlgorithm::kZLIB;
    } else if(compressionAlgo == "zstd") {
      config.compressionAlgorithm_ = ROOT::RCompressionSetting::EAlgorithm::kZSTD;
    } else if(compressionAlgo == "lz4") {
      config.compressionAlgorithm_ = ROOT::RCompressionSetting::EAlgorithm::kLZ4;
    } else {
      std::cout <<"unknown compression algorithm '"<<compressionAlgo<<"'"<<std::endl;
      return std::nullopt;
    }
    config.verbose_ = params.get<int>("verbose", config.verbose_);
    config.maxUnzippedPageSize_ = params.get<std::size_t>("maxUnzippedPageSize", config.maxUnzippedPageSize_);
    config.approxZippedClusterSize_ = params.get<std::size_t>("approxZippedClusterSize", config.approxZippedClusterSize_);
    config.maxUnzippedClusterSize_ = params.get<std::size_t>("maxUnzippedClusterSize", config.maxUnzippedClusterSize_);
    config.pageBufferBudget_ = params.get<std::size_t>("pageBufferBudget", config.pageBufferBudget_);
    config.useBufferedWrite_ = params.get<bool>("useBufferedWrite", config.useBufferedWrite_);
    config.useDirectIO_ = params.get<bool>("useDirectIO", config.useDirectIO_);
    config.enablePageChecksums_ = params.get<bool>("enablePageChecksums", config.enablePageChecksums_);
    config.printEstimateWriteMemoryUsage_ = params.get<bool>("printEstimateWriteMemoryUsage", config.printEstimateWriteMemoryUsage_);
    return std::make_pair(*fileName,config);
  }

  ROOT::RNTupleWriteOptions writeOptionsFrom(RNTupleOutputerConfig const& config) {
    ROOT::RNTupleWriteOptions writeOptions;
    writeOptions.SetCompression(config.compressionAlgorithm_, config.compressionLevel_);
    writeOptions.SetMaxUnzippedPageSize(config.maxUnzippedPageSize_);
    writeOptions.SetApproxZippedClusterSize(config.approxZippedClusterSize_);
    writeOptions.SetMaxUnzippedClusterSize(config.maxUnzippedClusterSize_);
    writeOptions.SetPageBufferBudget(config.pageBufferBudget_);
    writeOptions.SetUseBufferedWrite(config.useBufferedWrite_);
    writeOptions.SetUseDirectIO(config.useDirectIO_);
    writeOptions.SetEnablePageChecksums(config.enablePageChecksums_);

    return writeOptions;
  }

}
