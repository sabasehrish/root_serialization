#if !defined(RootEventOutputer_h)
#define RootEventOutputer_h

#include <vector>
#include <string>
#include <cstdint>
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializeStrategy.h"
#include "DataProductRetriever.h"

#include "SerialTaskQueue.h"

namespace cce::tf {
class RootEventOutputer :public OutputerBase {
 public:
  enum class Compression {kNone, kLZ4, kZSTD};
  enum class Serialization {kRoot, kRootUnrolled};
 RootEventOutputer(std::string const& iFileName, unsigned int iNLanes, Compression iCompression, int iCompressionLevel, 
                   Serialization iSerialization, int autoFlush, int maxVirtualSize );
 ~RootEventOutputer();

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return true;}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  
  void printSummary() const final;

 private:
  static inline size_t bytesToWords(size_t nBytes) {
    return nBytes/4 + ( (nBytes % 4) == 0 ? 0 : 1);
  }

  void output(EventIdentifier const& iEventID, SerializeStrategy const& iSerializers, std::vector<uint32_t> const& iBuffer);
  void writeMetaData(SerializeStrategy const& iSerializers);

  std::vector<uint32_t> writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const;

  std::vector<uint32_t> compressBuffer(unsigned int iReserveFirstNWords, unsigned int iPadding, std::vector<uint32_t> const& iBuffer, int& oCompressedSize) const;

  std::vector<uint32_t> lz4CompressBuffer(unsigned int iReserveFirstNWords, unsigned int iPadding, std::vector<uint32_t> const& iBuffer, int& oCompressedSize) const;
  std::vector<uint32_t> zstdCompressBuffer(unsigned int iReserveFirstNWords, unsigned int iPadding, std::vector<uint32_t> const& iBuffer, int& oCompressedSize) const;
  std::vector<uint32_t> noCompressBuffer(unsigned int iReserveFirstNWords, unsigned int iPadding, std::vector<uint32_t> const& iBuffer, int& oCompressedSize) const;

private:
  TFile file_;

  TTree* eventsTree_;

  mutable SerialTaskQueue queue_;
  mutable std::vector<SerializeStrategy> serializers_;
  mutable std::vector<uint32_t> eventBlob_;
  EventIdentifier eventID_;
  Compression compression_;
  int compressionLevel_;
  Serialization serialization_;
  bool firstTime_ = true;
  mutable std::chrono::microseconds serialTime_;
  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_;
};
}
#endif
