#if !defined(PDSOutputer_h)
#define PDSOutputer_h

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializerWrapper.h"
#include "DataProductRetriever.h"

#include "SerialTaskQueue.h"

class PDSOutputer :public OutputerBase {
 public:
 PDSOutputer(std::string const& iFileName, unsigned int iNLanes): file_(iFileName, std::ios_base::out| std::ios_base::binary),
    serializers_{std::size_t(iNLanes)}{
}

  void setupForLane(unsigned int iLaneIndex, std::vector<DataProductRetriever> const& iDPs) final;

  void productReadyAsync(unsigned int iLaneIndex, DataProductRetriever const& iDataProduct, TaskHolder iCallback) const final;
  bool usesProductReadyAsync() const final {return true;}

  void outputAsync(unsigned int iLaneIndex, EventIdentifier const& iEventID, TaskHolder iCallback) const final;
  
  void printSummary() const final;

 private:
  static inline size_t bytesToWords(size_t nBytes) {
    return nBytes/4 + ( (nBytes % 4) == 0 ? 0 : 1);
  }

  void output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers);
  void writeFileHeader(std::vector<SerializerWrapper> const& iSerializers);

  void writeEventHeader(EventIdentifier const& iEventID);
  void writeDataProducts(std::vector<SerializerWrapper> const& iSerializers);

private:
  std::ofstream file_;

  mutable SerialTaskQueue queue_;
  std::vector<std::pair<std::string, uint32_t>> dataProductIndices_;
  mutable std::vector<std::vector<SerializerWrapper>> serializers_;
  bool firstTime_ = true;
};

#endif
