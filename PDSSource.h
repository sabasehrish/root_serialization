#if !defined(PDSSource_h)
#define PDSSource_h

#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <fstream>


#include "SourceBase.h"
#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "pds_reading.h"

namespace cce::tf {
class PDSDelayedRetriever : public DelayedProductRetriever {
  void getAsync(DataProductRetriever&, int index, TaskHolder) override {}
};

class PDSSource : public SourceBase {
public:
  PDSSource(std::string const& iName);
  PDSSource(PDSSource&&) = default;
  PDSSource(PDSSource const&) = default;
  ~PDSSource();

  size_t numberOfDataProducts() const final {return dataProducts_.size();}
  std::vector<DataProductRetriever>& dataProducts() final { return dataProducts_; }
  EventIdentifier eventIdentifier() final { return eventID_;}

  using buffer_iterator = std::vector<std::uint32_t>::const_iterator;
private:
  bool readEvent(long iEventIndex) final; //returns true if an event was read
  bool readEventContent();

  pds::Compression compression_;
  std::ifstream file_;
  long presentEventIndex_ = 0;
  EventIdentifier eventID_;
  std::vector<DataProductRetriever> dataProducts_;
  std::vector<void*> dataBuffers_;
  PDSDelayedRetriever delayedRetriever_;
};
}
#endif
