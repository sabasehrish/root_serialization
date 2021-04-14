#include "PDSSource.h"
#include "TClass.h"

using namespace cce::tf;
using namespace cce::tf::pds;


bool PDSSource::readEvent(long iEventIndex) {
  while(iEventIndex != presentEventIndex_) {
    auto skipped = skipToNextEvent(file_);
    if(not skipped) {return false;}
    ++presentEventIndex_;
  }
  if(readEventContent()) {
    ++presentEventIndex_;
    return true;
  }
  return false;
}

bool PDSSource::readEventContent() {
  std::vector<uint32_t> buffer;
  if(not readCompressedEventBuffer(file_, eventID_, buffer)) {
    return false;
  }
  std::vector<uint32_t> uBuffer = uncompressEventBuffer(compression_, buffer);
  deserializeDataProducts(uBuffer.begin(), uBuffer.end(), dataProducts_);

  return true;
}

PDSSource::PDSSource(std::string const& iName) :
                 SourceBase(),
  file_{iName, std::ios_base::binary}
{
  auto productInfo = readFileHeader(file_, compression_);

  dataProducts_.reserve(productInfo.size());
  dataBuffers_.resize(productInfo.size(), nullptr);
  size_t index =0;
  for(auto const& pi : productInfo) {
    
    TClass* cls = TClass::GetClass(pi.className().c_str());
    dataBuffers_[index] = cls->New();
    assert(cls);
    dataProducts_.emplace_back(index,
			       &dataBuffers_[index],
                               pi.name(),
                               cls,
			       &delayedRetriever_);
    ++index;
  }
}

PDSSource::~PDSSource() {
  auto it = dataProducts_.begin();
  for( void * b: dataBuffers_) {
    it->classType()->Destructor(b);
    ++it;
  }
}

