#if !defined(pds_writer_h)
#define pds_writer_h

#include "pds_common.h"

#include <utility>
#include <vector>
#include <cstdint>

namespace cce::tf::pds {

  std::pair<std::vector<uint32_t>, int> compressBuffer(unsigned int iReserveFirstNWords, unsigned int iPadding, Compression iAlgorithm, int iCompressionLevel, std::vector<uint32_t> const& iBuffer);

  std::vector<char> compressBuffer(unsigned int iReserveFirstNWords, unsigned int iPadding, Compression iAlgorithm, int iCompressionLevel, std::vector<char> const& iBuffer);

}

#endif
