#if !defined(HDFEventOutputer_h)
#define HDFEventOutputer_h


#include <vector>
#include <string>
#include <cstdint>
#include <fstream>


#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializeStrategy.h"
#include "DataProductRetriever.h"
#include "pds_writer.h"

#include "SerialTaskQueue.h"

#include "HDFCxx.h"


namespace cce::tf {
  class HDFEventOutputer : public OutputerBase {
    public:
    HDFEventOutputer(std::string const& iFileName, unsigned int iNLanes, int iChunkSize, pds::Serialization iSerialization);
    HDFEventOutputer(HDFEventOutputer&&) = default;
    HDFEventOutputer(HDFEventOutputer const&) = default;

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return true;}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  
  void printSummary() const final;

 private:

  void output(EventIdentifier const& iEventID, SerializeStrategy const& iSerializers, std::vector<char> iBuffer, std::vector<uint32_t> iOffset);
  void writeFileHeader(SerializeStrategy const& iSerializers);
  std::pair<std::vector<uint32_t>,std::vector<char>> writeDataProductsToOutputBuffer(SerializeStrategy const& iSerializers) const;
private:
  hdf5::File file_;
  hdf5::Group group_;
  mutable SerialTaskQueue queue_;
  int chunkSize_;
  mutable std::vector<SerializeStrategy> serializers_;
  mutable std::pair<std::vector<uint32_t>, std::vector<char>> offsetsAndBlob_;
  bool firstEvent_ = true;
  pds::Serialization serialization_;
  mutable std::chrono::microseconds serialTime_;
  mutable std::atomic<std::chrono::microseconds::rep> parallelTime_;
  };    
}
#endif
