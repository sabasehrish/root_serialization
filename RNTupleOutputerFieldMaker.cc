#include "RNTupleOutputerFieldMaker.h"
#include <cassert>
#include <format>

namespace cce::tf {
  
  namespace {
    void noSplitField(ROOT::RFieldBase& iField) {
      auto const& typeName = iField.GetTypeName();
      if (typeName == "std::uint16_t") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kUInt16}});
      } else if (typeName == "std::uint32_t") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kUInt32}});
      } else if (typeName == "std::uint64_t") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kUInt64}});
      } else if (typeName == "std::int16_t") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kInt16}});
      } else if (typeName == "std::int32_t") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kInt32}});
      } else if (typeName == "std::int64_t") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kInt64}});
      } else if (typeName == "float") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kReal32}});
      } else if (typeName == "double") {
        iField.SetColumnRepresentatives({{ROOT::ENTupleColumnType::kReal64}});
      }
    }
    void applyNoSplitToSubFields(ROOT::RFieldBase& iField) {
      for (auto& subfield : iField) {
        noSplitField(subfield);
        applyNoSplitToSubFields(subfield);
      }
    }
  }  // namespace
  
  RNTupleOutputerFieldMaker::RNTupleOutputerFieldMaker(RNTupleOutputerConfig const& iConfig): noSplitFields_(iConfig.noSplitFields_) {}

  std::unique_ptr<ROOT::RFieldBase> RNTupleOutputerFieldMaker::make(std::string const& iName, std::string const& iType) const {
    auto field = ROOT::RFieldBase::Create(iName, iType).Unwrap();
    assert(field);
    if (noSplitFields_.size() == 1 and *noSplitFields_.begin() == "all") {
      noSplitField(*field);
      applyNoSplitToSubFields(*field);
    } else if(not noSplitFields_.empty()) {
      for (auto const& name : noSplitFields_) {
        if (name.starts_with(iName)) {
          bool found = false;
          for (auto& subfield : *field) {
            if (subfield.GetQualifiedFieldName() == name) {
              found = true;
              noSplitField(subfield);
              break;
            }
          }
          if (not found) {
            throw std::runtime_error(
              std::format("The data product was found but the requested subfield '{}' is not part of the class", iName));
          }
        }
      }
    
    }
    return field;
  }
}
