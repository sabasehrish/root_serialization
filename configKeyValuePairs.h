#if !defined(configKeyValuePairs_h)
#define configKeyValuePairs_h
#include <map>
#include <string>
#include <string_view>
namespace cce::tf {

  std::map<std::string,std::string> configKeyValuePairs(std::string_view iToParse);
}

#endif
