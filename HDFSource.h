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

using product_t = std::vector<char>;
using event_t = std::vector<int>;

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
  ProductInfo(std::string iName, uint32_t iIndex) : name_(std::move(iName)), index_{iIndex} {}

    uint32_t classIndex() const {return index_;}
    std::string const& name() const { return name_;}
    
    std::string name_;
    uint32_t index_;
  };
  
  size_t numberOfDataProducts() const final {return productInfo_.size();}
  std::vector<DataProductRetriever>& dataProducts() final {return dataProducts_;}
  EventIdentifier eventIdentifier() final { return eventID_;}
  static herr_t op_func (hid_t loc_id, const char *name, const H5L_info_t *info,void *operator_data);
  using buffer_iterator = std::vector<std::vector<char>>::const_iterator;

private: 
  std::vector<std::string> readClassNames();
  std::pair<long unsigned int, long unsigned int> getEventOffsets(long eventindex, std::string pname);
  bool readEvent(long iEventIndex) final; //returns true if an event was read
  void deserializeDataProducts(buffer_iterator, buffer_iterator);
  File file_;
  Group lumi_;
  EventIdentifier eventID_;
  std::vector<DataProductRetriever> dataProducts_; 
  std::vector<void*> dataBuffers_;  
  inline static std::vector<ProductInfo> productInfo_;
  std::vector<std::string> classnames_;
  HDFDelayedRetriever delayedRetriever_;
  inline static uint32_t dpid_; 
};
}
#endif
