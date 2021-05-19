#ifndef canvas_Persistency_Provenance_Timestamp_h
#define canvas_Persistency_Provenance_Timestamp_h

#include <cstdint>
#include <string>

namespace art {
  using TimeValue_t = std::uint64_t;

  class Timestamp {
  public:
    constexpr Timestamp(TimeValue_t const iValue)
      : timeLow_{static_cast<std::uint32_t>(lowMask() & iValue)}
      , timeHigh_{static_cast<std::uint32_t>(iValue >> 32)}
    {}

    constexpr Timestamp()
      : timeLow_{invalidTimestamp().timeLow_}
      , timeHigh_{invalidTimestamp().timeHigh_}
    {}

    // ---------- static member functions --------------------
    static constexpr Timestamp
    invalidTimestamp()
    {
      return Timestamp{0};
    }

  private:
    std::uint32_t timeLow_;
    std::uint32_t timeHigh_;

    static constexpr TimeValue_t
    lowMask()
    {
      return 0xFFFFFFFF;
    }
  };

}
#endif /* canvas_Persistency_Provenance_Timestamp_h */

// Local Variables:
// mode: c++
// End:
