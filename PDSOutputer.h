#if !defined(PDSOutputer_h)
#define PDSOutputer_h

#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <fstream>

#include "OutputerBase.h"
#include "EventIdentifier.h"
#include "SerializerWrapper.h"

#include "SerialTaskQueue.h"

class PDSOutputer :public OutputerBase {
 public:
  PDSOutputer(std::string const& iFileName): file_(iFileName, std::ios_base::out| std::ios_base::binary) {}

  void outputAsync(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers, TaskHolder iCallback) const final {
    queue_.push(*iCallback.group(), [this, iEventID, &iSerializers, callback=std::move(iCallback)]() mutable {
	const_cast<PDSOutputer*>(this)->output(iEventID, iSerializers);
	callback.doneWaiting();
      });
  }
  
 private:
  static inline size_t bytesToWords(size_t nBytes) {
    return nBytes/4 + ( (nBytes % 4) == 0 ? 0 : 1);
  }

  void output(EventIdentifier const& iEventID, std::vector<SerializerWrapper> const& iSerializers) {
    if(firstTime_) {
      writeFileHeader(iSerializers);
      firstTime_ = false;
    }
    using namespace std::string_literals;

    std::cout <<"   run:"s+std::to_string(iEventID.run)+" lumi:"s+std::to_string(iEventID.lumi)+" event:"s+std::to_string(iEventID.event)+"\n"<<std::flush;

    writeEventHeader(iEventID);
    writeDataProducts(iSerializers);
    /*
    for(auto& s: iSerializers) {
      std::cout<<"   "s+s.name()+" size "+std::to_string(s.blob().size())+"\n" <<std::flush;
    }
    */
  }

  void writeFileHeader(std::vector<SerializerWrapper> const& iSerializers) {
    std::set<std::string> typeNamesSet;
    for(auto const& w: iSerializers) {
      std::string n(w.className());
      n.push_back('\0');
      auto it = typeNamesSet.insert(n);
    }
    std::vector<std::string> typeNames(typeNamesSet.begin(), typeNamesSet.end());
    typeNamesSet.clear();
    size_t nCharactersInTypeNames = 0U;
    for(auto const& n: typeNames) {
      nCharactersInTypeNames += n.size();
    }
    
    std::vector<std::pair<uint32_t, std::string>> dataProducts;
    dataProducts.reserve(iSerializers.size());
    dataProductIndices_.reserve(iSerializers.size());
    size_t nCharactersInDataProducts = 0U;
    size_t index = 0;
    for(auto const& s: iSerializers) {
      auto itFind = std::lower_bound(typeNames.begin(), typeNames.end(), s.className());
      std::string name{s.name()};
      name.push_back('\0');
      dataProducts.emplace_back(itFind - typeNames.begin(), name);
      dataProductIndices_.emplace_back(name,index++);
      //pad to 32 bit size
      while( 0 != dataProducts.back().second.size() % 4) {
        dataProducts.back().second.push_back('\0');
      }
      nCharactersInDataProducts += 4 + dataProducts.back().second.size();
    }

    std::array<char, 8> transitions = {'E','v','e','n','t','\0','\0','\0'};

    size_t bufferPosition = 0;
    std::vector<uint32_t> buffer;
    const auto nWordsInTypeNames = bytesToWords(nCharactersInTypeNames);
    buffer.resize(1+transitions.size()/4+1+nWordsInTypeNames+1+1+nCharactersInDataProducts/4);

    //The different record types stored
    buffer[bufferPosition++] = transitions.size()/4;
    std::memcpy(reinterpret_cast<char*>(buffer.data()+bufferPosition), transitions.data(), transitions.size());
    bufferPosition += transitions.size()/4;
    
    //The 'top level' types stored in the file
    buffer[bufferPosition++] = nWordsInTypeNames;
    
    size_t bufferPositionInChars = bufferPosition*4;
    for(auto const& t: typeNames) {
      std::memcpy(reinterpret_cast<char*>(buffer.data())+bufferPositionInChars, t.data(), t.size());
      bufferPositionInChars+=t.size();
    }
    //std::cout <<bufferPositionInChars<<" "<<bufferPosition*4<<" "<<bufferPositionInChars-bufferPosition*4<<" "<<nCharactersInTypeNames<<std::endl;
    assert(bufferPositionInChars-bufferPosition*4 == nCharactersInTypeNames);

    bufferPosition += nWordsInTypeNames;

    //Information about types that are not at the 'top level' (none for now)
    buffer[bufferPosition++] = 0;

    //The different data products to be stored
    buffer[bufferPosition++] = dataProducts.size();
    for(auto const& dp : dataProducts) {
      buffer[bufferPosition++] = dp.first;
      std::memcpy(reinterpret_cast<char*>(buffer.data()+bufferPosition), dp.second.data(), dp.second.size());
      assert(0 == dp.second.size() % 4);
      bufferPosition += dp.second.size()/4;
    }

    {
      //The file type identifier
      const uint32_t id = 3141592*256+1;
      file_.write(reinterpret_cast<char const*>(&id), 4);
    }
    {
      //The 'unique' file id, just dummy for now
      const uint32_t fileID = 0;
      file_.write(reinterpret_cast<char const*>(&fileID), 4);     
    }

    //The size of the header buffer in words (excluding first 3 words)
    const uint32_t bufferSize = buffer.size();
    file_.write(reinterpret_cast<char const*>(&bufferSize), 4);

    file_.write(reinterpret_cast<char const*>(buffer.data()), bufferSize*4);
    //for(auto v: buffer) {
    //  file_.write(reinterpret_cast<char const*>(&v), sizeof(v));
    //}

    //The size of the header buffer in words (excluding first 3 words)
    file_.write(reinterpret_cast<char const*>(&bufferSize), 4);
  }

  void writeEventHeader(EventIdentifier const& iEventID) {
    constexpr unsigned int headerBufferSizeInWords = 5;
    std::array<uint32_t,headerBufferSizeInWords> buffer;
    buffer[0] = 0; //Record index for Event
    buffer[1] = iEventID.run;
    buffer[2] = iEventID.lumi;
    buffer[3] = (iEventID.event >> 32) & 0xFFFFFFFF;
    buffer[4] = iEventID.event & 0xFFFFFFFF;
    file_.write(reinterpret_cast<char*>(buffer.data()), headerBufferSizeInWords*4);
  }
  void writeDataProducts(std::vector<SerializerWrapper> const& iSerializers) {
    //Calculate buffer size needed
    uint32_t recordSize = 0;
    for(auto const& s: iSerializers) {
      recordSize +=1+1;
      auto const blobSize = s.blob().size();
      recordSize += bytesToWords(blobSize); //handles padding
    }
    //initialize with 0
    std::vector<uint32_t> buffer(size_t(recordSize+2), 0);

    uint32_t bufferIndex = 0;
    buffer[bufferIndex++] = recordSize;
    uint32_t dataProductIndex = 0;
    for(auto const& s: iSerializers) {
      buffer[bufferIndex++]=dataProductIndex++;
      auto const blobSize = s.blob().size();
      uint32_t sizeInWords = bytesToWords(blobSize);
      buffer[bufferIndex++]=sizeInWords;
      std::copy(s.blob().begin(), s.blob().end(), reinterpret_cast<char*>( &(*(buffer.begin()+bufferIndex)) ) );
      bufferIndex += sizeInWords;
    }
    buffer[bufferIndex++] = recordSize;
    assert(buffer.size() == bufferIndex);

    file_.write(reinterpret_cast<char*>(buffer.data()), buffer.size()*4);
  }

private:
  std::ofstream file_;

  mutable SerialTaskQueue queue_;
  std::vector<std::pair<std::string, uint32_t>> dataProductIndices_;
  bool firstTime_ = true;
};

#endif
