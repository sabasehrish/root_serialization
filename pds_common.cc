#include "pds_common.h"
#include <optional>
#include <string_view>

namespace cce::tf::pds {

  std::optional<Compression> toCompression(std::string_view compressionName) {
    
    if(compressionName == "" or compressionName =="None") {
      return pds::Compression::kNone;
    } else if (compressionName == "LZ4") {
      return pds::Compression::kLZ4;
    } else if (compressionName == "ZSTD") {
      return pds::Compression::kZSTD;
    } 
    return {};
  }

  const char* name(Compression compression) {
    //Need to guarantee that a length of 4 is always
    // valid (that can include the trailing \0)
    switch(compression) {
    case Compression::kNone:
      {
        return "None";
      }
    case Compression::kLZ4:
      {
        return "LZ4";
      }
    case Compression::kZSTD:
      {
        return "ZSTD";
      }
    }
    //should never get here
    return "";
  }

  std::optional<Serialization> toSerialization(std::string_view serializationName) {
    if(serializationName == "" or serializationName=="ROOT") {
      return pds::Serialization::kRoot;
    } else if(serializationName == "ROOTUnrolled" or serializationName=="Unrolled") {
      return pds::Serialization::kRootUnrolled;
    }
    return {};
  }
}
