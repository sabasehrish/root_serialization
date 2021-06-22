#if !defined(pds_reading_h)
#define pds_reading_h

#include <istream>
#include <vector>

#include "DeserializeStrategy.h"
#include "EventIdentifier.h"
#include "DataProductRetriever.h"

namespace cce::tf::pds {

  uint32_t readword(std::istream& iFile);

  uint32_t readwordNoCheck(std::istream& iFile);

  std::vector<uint32_t> readWords(std::istream& iFile, uint32_t bufferSize) ;


  struct ProductInfo{
  ProductInfo(std::string iName, uint32_t iIndex, std::string iClassName) : name_(std::move(iName)), index_{iIndex}, className_{iClassName} {}
    
    uint32_t classIndex() const {return index_;}
    std::string const& name() const { return name_;}
    std::string const& className() const { return className_;}
    std::string className_;
    std::string name_;
    uint32_t index_;
  };
  
  enum class Compression {kNone, kLZ4, kZSTD};
  enum class Serialization {kRoot, kRootUnrolled};

  std::vector<ProductInfo> readFileHeader(std::istream&, Compression&, Serialization&);

  constexpr size_t kEventHeaderSizeInWords = 5;
  bool skipToNextEvent(std::istream&); //returns true if an event was skipped
  bool readCompressedEventBuffer(std::istream&, EventIdentifier&, std::vector<uint32_t>& buffer);
  std::vector<uint32_t> uncompressEventBuffer(pds::Compression, std::vector<uint32_t> const& buffer);
  void deserializeDataProducts(std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator, std::vector<DataProductRetriever>&, DeserializeStrategy const&);

}

#endif
