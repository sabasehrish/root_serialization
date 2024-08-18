#include "configKeyValuePairs.h"
#include <iostream>

namespace cce::tf {

  namespace {
    std::string::size_type keyValueEnd(std::string_view iToParse, std::string::size_type iStart) {
      auto colonPos = iToParse.find(':', iStart);
      auto quotePos = iToParse.find('"', iStart);
      if (quotePos < colonPos) {
        //find ending quote
        auto lastQuotePos = iToParse.find('"', quotePos+1);
        if (lastQuotePos == std::string::npos) {
          std::cerr <<"Missing end quote in configuration";
          exit(1);
        }
        colonPos = iToParse.find(':', lastQuotePos);
      }
      return colonPos;
    }
    std::string stripQuoteFromValue(std::string iValue) {
      if( iValue.empty() or iValue[0] != '"') {
        return iValue;
      }
      return iValue.substr(1,iValue.size()-2);
    }
  }
  
  ConfigKeyValueMap configKeyValuePairs(std::string_view iToParse) {
    ConfigKeyValueMap keyValues;
    std::string::size_type start=0;
    std::string::size_type pos;

    bool firstEntry = true;
    do {
      auto pos = keyValueEnd(iToParse, start);
      std::string keyValue;
      if(pos != std::string::npos) {
	keyValue = iToParse.substr(start,pos-start);
      } else {
	keyValue = iToParse.substr(start);
      }
      start = pos;
      if(pos != std::string::npos) {
	++start;
      }
      if(keyValue.empty()) {
	break;
      }
      if( (pos = keyValue.find('=')) != std::string::npos ) {
	keyValues.emplace(keyValue.substr(0,pos), stripQuoteFromValue(keyValue.substr(pos+1)));
      } else {
        if(firstEntry) {
          //this might be a file name
          auto pos = keyValue.find('/',0);
          if(pos != std::string::npos) {
            keyValues.emplace("fileName",stripQuoteFromValue(keyValue));
          } else {
            pos = keyValue.find('.',0);
            if(pos != std::string::npos) {
              keyValues.emplace("fileName",stripQuoteFromValue(keyValue));
            } else {
              keyValues.emplace(keyValue,"");
            }
          }
        } else {
          keyValues.emplace(keyValue,"");
        }
      }
      firstEntry = false;
    } while(start != std::string::npos);
    return keyValues;
  }
}
