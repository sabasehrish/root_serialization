#if !defined(HDFSource_h)
#define HDFSource_h

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"
#include "SourceBase.h"

#include "HDFCxx.h"

namespace cce::tf {
class HDFDelayedRetriever : public DelayedProductRetriever {
  void getAsync(DataProductRetriever&, int index, TaskHolder) override {}
};

class HDFSource : public SourceBase {
public:
  HDFSource(std::string const& iName);
  HDFSource(HDFSource&&) = default;
  HDFSource(HDFSource const&) = default;
  ~HDFSource();

  struct ProductInfo{
  ProductInfo(std::string iName, size_t iIndex) : name_(std::move(iName)), index_{iIndex} {}

    size_t classIndex() const {return index_;}
    std::string const& name() const { return name_;}
    
    std::string name_;
    size_t index_;
  };
  
  size_t numberOfDataProducts() const final {return productInfos_.size();}
  std::vector<DataProductRetriever>& dataProducts() final {return dataProducts_;}
  EventIdentifier eventIdentifier() final { return eventID_;}
  using buffer_iterator = std::vector<std::vector<char>>::const_iterator;

private: 
  std::vector<std::string> readClassNames();
  std::pair<long unsigned int, long unsigned int> getEventOffsets(long eventindex, std::string const& pname);
  bool readEvent(long iEventIndex) final; //returns true if an event was read
  void deserializeDataProducts(buffer_iterator, buffer_iterator);
  hdf5::File file_;
  hdf5::Group lumi_;
  EventIdentifier eventID_;
  std::vector<DataProductRetriever> dataProducts_; 
  std::vector<void*> dataBuffers_;  
  std::vector<ProductInfo> productInfos_;
  std::vector<std::string> classnames_;
  HDFDelayedRetriever delayedRetriever_;
};
}
#endif
