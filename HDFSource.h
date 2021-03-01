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

using namespace HighFive;
using product_t = std::vector<char>;
using event_t = std::vector<int>;

namespace cce::tf {
class HDFSource : public SourceBase {
public:
  HDFSource(std::string const& iName);
  HDFSource(HDFSource&&) = default;
  HDFSource(HDFSource const&) = default;
  size_t numberOfDataProducts() const final {return productnames_.size();}
  std::vector<DataProductRetriever>& dataProducts() final {return products_;}
  EventIdentifier eventIdentifier() final { return eventID_;}
//  event_t& events() final {return events_};
//  std::vector<std::string>& classNames() final {return classnames_};
//  std::vector<std::string>& productNames() final {return productnames_};
  //bool readEvent(long iEventIndex) final;
private: 
  bool readEvent(long iEventIndex) final;
  //long numberOfEvents();

  HighFive::File file_;
  long presentEventIndex_ = 0;
  event_t events_;
  EventIdentifier eventID_;
  std::vector<DataProductRetriever> products_;
  std::vector<std::string> classnames_;
  std::vector<std::string> productnames_;

  //events data set
  //
};
}
#endif
