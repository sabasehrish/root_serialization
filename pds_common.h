#if !defined(pds_common_h)
#define pds_common_h

namespace cce::tf::pds {
  enum class Compression {kNone, kLZ4, kZSTD};
  enum class Serialization {kRoot, kRootUnrolled};
}
#endif
