#if !defined(RootBatchEventsOutputer_h)
#define RootBatchEventsOutputer_h

#include <vector>
#include <string>
#include <cstdint>
#include <tuple>
#include <atomic>
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializeStrategy.h"
#include "DataProductRetriever.h"
#include "pds_writer.h"

#include "SerialTaskQueue.h"

namespace cce::tf {
class RootBatchEventsOutputer :public OutputerBase {
 public:
  RootBatchEventsOutputer(std::string const& iFileName, unsigned int iNLanes, pds::Compression iCompression, int iCompressionLevel, 
                          pds::Serialization iSerialization, int autoFlush, int maxVirtualSize,
                          std::string const& iTFileCompression, int iTFileCompressionLevel,
                          uint32_t iBatchSize);
 ~RootBatchEventsOutputer();

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return true;}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  
  void printSummary() const final;

 private:
  void finishBatchAsync(unsigned int iBatchIndex, TaskHolder iCallback);

  void output(std::vector<EventIdentifier> iEventIDs, std::vector<char>  iBuffer, std::vector<uint32_t> iOffset);
  void writeMetaData(SerializeStrategy const& iSerializers);

  std::pair<std::vector<uint32_t>,std::vector<char>> writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const;

  std::vector<char> compressBuffer(std::vector<char> const& iBuffer) const;

private:
  mutable TFile file_;

  TTree* eventsTree_;

  mutable SerialTaskQueue queue_;
  mutable std::vector<SerializeStrategy> serializers_;

  //objects used by the TBranches
  mutable std::pair<std::vector<uint32_t>, std::vector<char>> offsetsAndBlob_;
  mutable std::vector<EventIdentifier> eventIDs_;

  //This is used as a circular buffer of length nLanes but only entries being used exist
  using EventInfo = std::tuple<EventIdentifier, std::vector<uint32_t>, std::vector<char>>;
  mutable std::vector<std::atomic<std::vector<EventInfo>*>> eventBatches_;
  mutable std::vector<std::atomic<uint32_t>> waitingEventsInBatch_;

  mutable std::atomic<uint64_t> presentEventEntry_;

  uint32_t batchSize_;
  pds::Compression compression_;
  int compressionLevel_;
  pds::Serialization serialization_;
  mutable std::chrono::microseconds serialTime_;
  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_;
};
}
#endif
