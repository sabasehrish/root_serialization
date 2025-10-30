#if !defined(RNTupleOutputerFieldMaker_h)
#define RNTupleOutputerFieldMaker_h

#include <ROOT/RNTupleWriter.hxx>
#include "RNTupleOutputerConfig.h"

#include <string>
#include <set>

namespace cce::tf {
  class RNTupleOutputerFieldMaker {
  public:
    RNTupleOutputerFieldMaker(RNTupleOutputerConfig const&);

    std::unique_ptr<ROOT::RFieldBase> make(std::string const& iName, std::string const& iType) const;
  private:
    std::set<std::string> noSplitFields_;
  };
}


#endif
