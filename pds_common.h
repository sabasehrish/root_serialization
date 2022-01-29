#if !defined(pds_common_h)
#define pds_common_h

#include <optional>
#include <string_view>

namespace cce::tf::pds {
  enum class Compression {kNone, kLZ4, kZSTD};
  enum class Serialization {kRoot, kRootUnrolled};

  //returned value is guaranteed to have starting 4 
  // characters be unique for each compression factor
  // (the 4 may or may not include the trailing \0
  const char* name(Compression compression);

  std::optional<Compression> toCompression(std::string_view);
  std::optional<Serialization> toSerialization(std::string_view);  
}
#endif
