#if !defined(HDFSource_h)
#define HDFSource_h


#include <string>
#include <memory>
#include <optional>
#include <vector>

#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5DataType.hpp>
#include <highfive/H5File.hpp>

#include "SourceBase.h"
#include "DataProductRetriever.h"
#include "DelayedProductRetriever.h"

using namespace HighFive;
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
  
  
  using buffer_iterator = std::vector<std::vector<char>>::const_iterator;

private: 
  //std::vector<ProductInfo> readProductInfo(buffer_iterator&, buffer_iterator);
  std::vector<std::string> readClassNames();
  std::vector<ProductInfo> readProductInfo();
  std::pair<long unsigned int, long unsigned int> getEventOffsets(long eventindex, std::string pname);
  bool readEvent(long iEventIndex) final; //returns true if an event was read
  void deserializeDataProducts(buffer_iterator, buffer_iterator);
  
  HighFive::File file_;
  HighFive::Group lumi_;
  EventIdentifier eventID_;
  std::vector<DataProductRetriever> dataProducts_; 
  std::vector<void*> dataBuffers_; //data read directly from HDF dataset 
  std::vector<ProductInfo> productInfo_;
  std::vector<std::string> classnames_;
  HDFDelayedRetriever delayedRetriever_;
};
}
#endif
