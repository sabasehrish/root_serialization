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
#include "pds_writer.h"

#include "SerialTaskQueue.h"

namespace cce::tf {
class RootEventOutputer :public OutputerBase {
 public:
  RootEventOutputer(std::string const& iFileName, unsigned int iNLanes, pds::Compression iCompression, int iCompressionLevel, 
                    pds::Serialization iSerialization, int autoFlush, int maxVirtualSize,
                    std::string const& iTFileCompression, int iTFileCompressionLevel);
 ~RootEventOutputer();

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return true;}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  
  void printSummary() const final;

 private:
  void output(EventIdentifier const& iEventID, SerializeStrategy const& iSerializers, std::vector<char>  iBuffer, std::vector<uint32_t> iOffset);
  void writeMetaData(SerializeStrategy const& iSerializers);

  std::pair<std::vector<uint32_t>,std::vector<char>> writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const;

  std::vector<char> compressBuffer(std::vector<char> const& iBuffer) const;

private:
  mutable TFile file_;

  TTree* eventsTree_;

  mutable SerialTaskQueue queue_;
  mutable std::vector<SerializeStrategy> serializers_;
  mutable std::pair<std::vector<uint32_t>, std::vector<char>> offsetsAndBlob_;
  EventIdentifier eventID_;
  pds::Compression compression_;
  int compressionLevel_;
  pds::Serialization serialization_;
  mutable std::chrono::microseconds serialTime_;
  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_;
};
}
#endif
