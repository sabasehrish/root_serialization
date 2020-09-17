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

class PDSDelayedRetriever : public DelayedProductRetriever {
  void getAsync(int index, TaskHolder) override {}
};

class PDSSource : public SourceBase {
public:
  PDSSource(std::string const& iName, unsigned long long iNEvents);
  PDSSource(PDSSource&&) = default;
  PDSSource(PDSSource const&) = default;
  ~PDSSource();

  std::vector<DataProductRetriever>& dataProducts() final { return dataProducts_; }
  EventIdentifier eventIdentifier() final { return eventID_;}

  struct ProductInfo{
  ProductInfo(std::string iName, uint32_t iIndex) : name_(std::move(iName)), index_{iIndex} {}

    uint32_t classIndex() const {return index_;}
    std::string const& name() const { return name_;}
    
    std::string name_;
    uint32_t index_;
  };

  using buffer_iterator = std::vector<std::uint32_t>::const_iterator;
private:
  static constexpr size_t kEventHeaderSizeInWords = 5;
  uint32_t readPreamble(); //returns header buffer size in words
  std::vector<std::string> readStringsArray(buffer_iterator&, buffer_iterator);
  std::vector<std::string> readRecordNames(buffer_iterator&, buffer_iterator);
  std::vector<std::string> readTypes(buffer_iterator&, buffer_iterator);
  std::vector<ProductInfo> readProductInfo(buffer_iterator&, buffer_iterator);
  uint32_t readword();
  uint32_t readwordNoCheck();
  std::vector<uint32_t> readWords(uint32_t);
  bool readEvent(long iEventIndex) final; //returns true if an event was read
  bool skipToNextEvent(); //returns true if an event was skipped
  bool readEventContent();
  void deserializeDataProducts(buffer_iterator, buffer_iterator);

  std::ifstream file_;
  long presentEventIndex_ = 0;
  EventIdentifier eventID_;
  std::vector<DataProductRetriever> dataProducts_;
  std::vector<void*> dataBuffers_;
  PDSDelayedRetriever delayedRetriever_;
};
#endif
