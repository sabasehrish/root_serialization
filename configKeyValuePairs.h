#if !defined(configKeyValuePairs_h)
#define configKeyValuePairs_h
#include <map>
#include <string>
#include <string_view>
namespace cce::tf {

  using ConfigKeyValueMap = std::map<std::string, std::string, std::less<>>;
  ConfigKeyValueMap configKeyValuePairs(std::string_view iToParse);
}

#endif
