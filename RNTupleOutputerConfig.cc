#include "RNTupleOutputerConfig.h"

#include "ConfigurationParameters.h"
#include <iostream>

namespace cce::tf {
  namespace {
    std::set<std::string> parseSplitFields(std::string const& iFields) {
      //Eventually should given an option to read a file containing the fields
      std::set<std::string> fields;
      if (iFields.empty()) {
        return fields;
      }
      std::size_t lastComma = 0;
      std::size_t tokenStart = 0;
      while(std::string::npos != (lastComma = iFields.find(',',tokenStart))) {
        fields.insert(iFields.substr(tokenStart, lastComma-tokenStart));
        tokenStart = lastComma + 1;
      }
      fields.insert(iFields.substr(tokenStart, lastComma-tokenStart));
      return fields;
    }
  }
  
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
    config.initialUnzippedPageSize_ = params.get<std::size_t>("initialUnzippedPageSize", config.initialUnzippedPageSize_);
    config.approxZippedClusterSize_ = params.get<std::size_t>("approxZippedClusterSize", config.approxZippedClusterSize_);
    config.maxUnzippedClusterSize_ = params.get<std::size_t>("maxUnzippedClusterSize", config.maxUnzippedClusterSize_);
    config.pageBufferBudget_ = params.get<std::size_t>("pageBufferBudget", config.pageBufferBudget_);
    config.useBufferedWrite_ = params.get<bool>("useBufferedWrite", config.useBufferedWrite_);
    config.useDirectIO_ = params.get<bool>("useDirectIO", config.useDirectIO_);
    config.enablePageChecksums_ = params.get<bool>("enablePageChecksums", config.enablePageChecksums_);
    config.printEstimateWriteMemoryUsage_ = params.get<bool>("printEstimateWriteMemoryUsage", config.printEstimateWriteMemoryUsage_);
    config.noSplitFields_ = parseSplitFields(params.get<std::string>("noSplitFields",""));
    return std::make_pair(*fileName,config);
  }

  ROOT::RNTupleWriteOptions writeOptionsFrom(RNTupleOutputerConfig const& config) {
    ROOT::RNTupleWriteOptions writeOptions;
    writeOptions.SetCompression(config.compressionAlgorithm_, config.compressionLevel_);
    writeOptions.SetMaxUnzippedPageSize(config.maxUnzippedPageSize_);
    writeOptions.SetInitialUnzippedPageSize(config.initialUnzippedPageSize_);
    writeOptions.SetApproxZippedClusterSize(config.approxZippedClusterSize_);
    writeOptions.SetMaxUnzippedClusterSize(config.maxUnzippedClusterSize_);
    writeOptions.SetPageBufferBudget(config.pageBufferBudget_);
    writeOptions.SetUseBufferedWrite(config.useBufferedWrite_);
    writeOptions.SetUseDirectIO(config.useDirectIO_);
    writeOptions.SetEnablePageChecksums(config.enablePageChecksums_);

    return writeOptions;
  }

}
