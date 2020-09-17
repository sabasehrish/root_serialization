#if !defined(PDSSource_h)
#define PDSSource_h

#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <fstream>

#include "lz4.h"

#include "TClass.h"
#include "TBufferFile.h"

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

inline uint32_t PDSSource::readword() {
  int32_t word;
  file_.read(reinterpret_cast<char*>(&word), 4);
  assert(file_.rdstate() == std::ios_base::goodbit);
  return word;
}

inline uint32_t PDSSource::readwordNoCheck() {
  int32_t word;
  file_.read(reinterpret_cast<char*>(&word), 4);
  return word;
}

inline std::vector<uint32_t> PDSSource::readWords(uint32_t bufferSize) {
  std::vector<uint32_t> words(bufferSize);
  file_.read(reinterpret_cast<char*>(words.data()), 4*bufferSize);
  assert(file_.rdstate() == std::ios_base::goodbit);
  return words;  
}


inline uint32_t PDSSource::readPreamble() {
  std::array<uint32_t, 3> header;
  file_.read(reinterpret_cast<char*>(header.data()),3*4);
  assert(file_.rdstate() == std::ios_base::goodbit);

  assert(3141592*256+1 == header[0]);
  return header[2];
}

inline std::vector<std::string> PDSSource::readStringsArray(buffer_iterator& itBuffer, buffer_iterator itEnd) {
  assert(itBuffer!=itEnd);
  auto bufferSize = *(itBuffer++);

  std::vector<std::string> toReturn;
  if(bufferSize == 0) {
    return toReturn;
  }

  assert(itBuffer+bufferSize <= itEnd);

  const char* itChars = reinterpret_cast<const char*>(&(*itBuffer));
  itBuffer = itBuffer+bufferSize;

  auto itEndChars = itChars + bufferSize*4;
  auto nStrings = std::count(itChars, itEndChars,'\0');
  toReturn.reserve(nStrings);

  while(itChars != itEndChars) {
    std::string s(&(*itChars));
    if(s.empty()) {
      break;
    }
    itChars = itChars+s.size()+1;
    //std::cout <<s<<std::endl;
    toReturn.emplace_back(std::move(s));
  }
  //std::cout << (void*)itChars << " "<<&(*itBuffer)<<std::endl;
  assert(itChars <= reinterpret_cast<const char*>(&(*itBuffer)));
  return toReturn;
}

inline std::vector<std::string> PDSSource::readRecordNames(buffer_iterator& itBuffer, buffer_iterator itEnd) {
  return readStringsArray(itBuffer, itEnd);
}

inline std::vector<std::string> PDSSource::readTypes(buffer_iterator& itBuffer, buffer_iterator itEnd) {
  return readStringsArray(itBuffer, itEnd);
}

namespace {
  inline size_t bytesToWords(size_t nBytes) {
    return nBytes/4 + ( (nBytes % 4) == 0 ? 0 : 1);
  }
}

inline std::vector<PDSSource::ProductInfo> PDSSource::readProductInfo(buffer_iterator& itBuffer, buffer_iterator itEnd) {
  assert(itBuffer != itEnd);
  //should be a loop over records, but we only have 1 for now
  auto nDataProducts = *(itBuffer++);
  std::vector<ProductInfo> info;
  info.reserve(nDataProducts);

  for(int i=0; i< nDataProducts; ++i) {
    assert(itBuffer < itEnd);
    auto classIndex = *(itBuffer++);
    assert(itBuffer < itEnd);

    const char* itChars = reinterpret_cast<const char*>(&(*itBuffer));
    std::string name(itChars);
    itBuffer = itBuffer + bytesToWords(name.size()+1);
    assert(itBuffer <= itEnd);
    //std::cout <<name <<" "<<classIndex<<std::endl;
    info.emplace_back(std::move(name), classIndex);
  }

  return info;
}


inline bool PDSSource::readEvent(long iEventIndex) {
  while(iEventIndex != presentEventIndex_) {
    auto skipped = skipToNextEvent();
    if(not skipped) {return false;}
  }
  return readEventContent();
}

inline bool PDSSource::readEventContent() {
  //std::cout <<"readEventContent"<<std::endl;
  std::array<uint32_t, kEventHeaderSizeInWords+1> headerBuffer;
  file_.read(reinterpret_cast<char*>(headerBuffer.data()), (kEventHeaderSizeInWords+1)*4);
  if( file_.rdstate() & std::ios_base::eofbit) {
    return false;
  }
  assert(file_.rdstate() == std::ios_base::goodbit);

  int32_t bufferSize = headerBuffer[kEventHeaderSizeInWords];

  unsigned long long eventIDTopWord = headerBuffer[3];
  eventIDTopWord = eventIDTopWord <<32;
  unsigned long long eventID = eventIDTopWord+headerBuffer[4];
  eventID_ = {headerBuffer[1], headerBuffer[2], eventID};

  std::vector<uint32_t> buffer = readWords(bufferSize+1);

  int32_t crossCheckBufferSize = buffer[bufferSize];
  //std::cout <<bufferSize<<" "<<crossCheckBufferSize<<std::endl;
  assert(crossCheckBufferSize == bufferSize);

  ++presentEventIndex_;

  //lower 2 bits are the number of bytes used in the last word of the compressed sized
  int32_t uncompressedBufferSize = buffer[0]/4;
  int32_t bytesInLastWord = buffer[0] % 4;
  int32_t compressedBufferSizeInBytes = (bufferSize-1)*4 + (bytesInLastWord == 0? 0 : (-4+bytesInLastWord));
  //std::cout <<"compressed "<<compressedBufferSizeInBytes <<" uncompressed "<<uncompressedBufferSize*4<<" extra bytes "<<bytesInLastWord<<std::endl;
  std::vector<uint32_t> uBuffer(size_t(uncompressedBufferSize), 0);
  LZ4_decompress_safe(reinterpret_cast<char*>(&(*(buffer.begin()+1))), reinterpret_cast<char*>(uBuffer.data()),
                      compressedBufferSizeInBytes,
                      uncompressedBufferSize*4);

  deserializeDataProducts(uBuffer.begin(), uBuffer.end());

  return true;
}

void PDSSource::deserializeDataProducts(buffer_iterator it, buffer_iterator itEnd) {
  TBufferFile bufferFile{TBuffer::kRead};

  while(it < itEnd) {
    auto productIndex = *(it++);
    auto storedSize = *(it++);
   
    //std::cout <<"storedSize "<<storedSize<<" "<<storedSize*4<<std::endl;
    bufferFile.SetBuffer(const_cast<char*>(reinterpret_cast<char const*>(&*it)), storedSize*4, kFALSE);
    dataProducts_[productIndex].classType()->ReadBuffer(bufferFile, dataBuffers_[productIndex]);
    dataProducts_[productIndex].setSize(bufferFile.Length());
  //std::cout <<" size "<<bufferFile.Length()<<"\n";
    bufferFile.Reset();
    it = it+storedSize;
  }
  assert(it==itEnd);
}


inline bool PDSSource::skipToNextEvent() {
  file_.seekg(kEventHeaderSizeInWords*4, std::ios_base::cur);
  if( file_.rdstate() & std::ios_base::eofbit) {
    return false;
  }
  assert(file_.rdstate() == std::ios_base::goodbit);

  int32_t bufferSize = readwordNoCheck();
  if( file_.rdstate() & std::ios_base::eofbit) {
    return false;
  }

  file_.seekg(bufferSize*4,std::ios_base::cur);
  assert(file_.rdstate() == std::ios_base::goodbit);

  int32_t crossCheckBufferSize = readword();
  assert(crossCheckBufferSize == bufferSize);

  ++presentEventIndex_;
  return true;
}


inline PDSSource::PDSSource(std::string const& iName, unsigned long long iNEvents) :
                 SourceBase(iNEvents),
  file_{iName, std::ios_base::binary}
{
  auto bufferSize = readPreamble();
  
  //1 word beyond the buffer is the crosscheck value
  std::vector<uint32_t> buffer = readWords(bufferSize+1);
  auto itBuffer = buffer.cbegin();
  auto itEnd = buffer.cend();

  (void) readRecordNames(itBuffer, itEnd);
  auto types = readTypes(itBuffer, itEnd);
  //non-top level types
  readTypes(itBuffer, itEnd);
  auto productInfo = readProductInfo(itBuffer, itEnd);
  assert(itBuffer != itEnd);
  assert(itBuffer+1 == itEnd);
  //std::cout <<*itBuffer <<" "<<bufferSize<<std::endl;
  assert(*itBuffer == bufferSize);

  dataProducts_.reserve(productInfo.size());
  dataBuffers_.resize(productInfo.size(), nullptr);
  size_t index =0;
  for(auto const& pi : productInfo) {
    
    TClass* cls = TClass::GetClass(types[pi.classIndex()].c_str());
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

inline PDSSource::~PDSSource() {
  auto it = dataProducts_.begin();
  for( void * b: dataBuffers_) {
    it->classType()->Destructor(b);
    ++it;
  }
}

#endif
