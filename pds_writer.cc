#include "pds_writer.h"
#include <algorithm>
#include <iostream>

#include "lz4.h"
#include "zstd.h"

namespace {
  static inline size_t bytesToWords(size_t nBytes) {
    return nBytes/4 + ( (nBytes % 4) == 0 ? 0 : 1);
  }
  
  std::pair<std::vector<uint32_t>,int> lz4CompressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, std::vector<uint32_t> const& iBuffer) {
    int cSize = 0;
    auto const bound = LZ4_compressBound(iBuffer.size()*4);
    std::vector<uint32_t> cBuffer(bytesToWords(size_t(bound))+iLeadPadding+iTrailingPadding, 0);
    cSize = LZ4_compress_default(reinterpret_cast<char const*>(&(*iBuffer.begin())), reinterpret_cast<char*>(&(*(cBuffer.begin()+iLeadPadding))), iBuffer.size()*4, bound);
    cBuffer.resize(bytesToWords(cSize)+iLeadPadding+iTrailingPadding);
    return {cBuffer,cSize};
  }
  
  std::pair<std::vector<uint32_t>, int> noCompressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, std::vector<uint32_t> const& iBuffer) {
    int cSize = 0;
    auto const bound = iBuffer.size()*4;
    std::vector<uint32_t> cBuffer(iBuffer.size()+iLeadPadding+iTrailingPadding, uint32_t(0));
    cSize = bound;
    std::copy(iBuffer.begin(), iBuffer.end(), cBuffer.begin()+iLeadPadding);
    return {cBuffer, cSize};
  }
  
  std::pair<std::vector<uint32_t>, int> zstdCompressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, std::vector<uint32_t> const& iBuffer, int compressionLevel) {
    int cSize = 0;
    auto const bound = ZSTD_compressBound(iBuffer.size()*4);
    std::vector<uint32_t> cBuffer(bytesToWords(size_t(bound))+iLeadPadding+iTrailingPadding, 0);
    cSize = ZSTD_compress(&(*(cBuffer.begin()+iLeadPadding)), bound, &(*iBuffer.begin()),  iBuffer.size()*4, compressionLevel);
    if(ZSTD_isError(cSize)) {
      std::cout <<"ERROR in comparession "<<ZSTD_getErrorName(cSize)<<std::endl;
    }
    cBuffer.resize(bytesToWords(cSize)+iLeadPadding+iTrailingPadding);
    return {cBuffer,cSize};
  }


  std::vector<char> lz4CompressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, std::vector<char> const& iBuffer) {
    auto const bound = LZ4_compressBound(iBuffer.size());
    std::vector<char> cBuffer(bound+iLeadPadding+iTrailingPadding, 0);
    auto cSize = LZ4_compress_default(&(*iBuffer.begin()), &(*(cBuffer.begin()+iLeadPadding)), iBuffer.size(), bound);
    cBuffer.resize(cSize+iLeadPadding+iTrailingPadding);
    return cBuffer;
  }
  
  std::vector<char> noCompressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, std::vector<char> const& iBuffer) {
    std::vector<char> cBuffer(iBuffer.size()+iLeadPadding+iTrailingPadding, uint32_t(0));
    std::copy(iBuffer.begin(), iBuffer.end(), cBuffer.begin()+iLeadPadding);
    return cBuffer;
  }
  
  std::vector<char> zstdCompressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, std::vector<char> const& iBuffer, int compressionLevel) {
    int cSize = 0;
    auto const bound = ZSTD_compressBound(iBuffer.size());
    std::vector<char> cBuffer(bound+iLeadPadding+iTrailingPadding, 0);
    cSize = ZSTD_compress(&(*(cBuffer.begin()+iLeadPadding)), bound, &(*iBuffer.begin()),  iBuffer.size(), compressionLevel);
    if(ZSTD_isError(cSize)) {
      std::cout <<"ERROR in comparession "<<ZSTD_getErrorName(cSize)<<std::endl;
    }
    cBuffer.resize(cSize+iLeadPadding+iTrailingPadding);
    return cBuffer;
  }

}

namespace cce::tf::pds {
  
  std::pair<std::vector<uint32_t>, int> compressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, Compression iAlgorithm, int iCompressionLevel, std::vector<uint32_t> const& iBuffer) {

    switch(iAlgorithm) {
    case Compression::kLZ4 : {
      return lz4CompressBuffer(iLeadPadding,iTrailingPadding, iBuffer);
    }    
    case Compression::kNone : {
      return noCompressBuffer(iLeadPadding, iTrailingPadding, iBuffer);
    } 
    case Compression::kZSTD : {
      return zstdCompressBuffer(iLeadPadding, iTrailingPadding, iBuffer, iCompressionLevel);
    }
    default:
      return noCompressBuffer(iLeadPadding, iTrailingPadding, iBuffer);
      
    }
  }

  std::vector<char> compressBuffer(unsigned int iLeadPadding, unsigned int iTrailingPadding, Compression iAlgorithm, int iCompressionLevel, std::vector<char> const& iBuffer) {

    switch(iAlgorithm) {
    case Compression::kLZ4 : {
      return lz4CompressBuffer(iLeadPadding,iTrailingPadding, iBuffer);
    }    
    case Compression::kNone : {
      return noCompressBuffer(iLeadPadding, iTrailingPadding, iBuffer);
    } 
    case Compression::kZSTD : {
      return zstdCompressBuffer(iLeadPadding, iTrailingPadding, iBuffer, iCompressionLevel);
    }
    default:
      return noCompressBuffer(iLeadPadding, iTrailingPadding, iBuffer);
      
    }
  }

}
