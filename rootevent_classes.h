#ifndef rootevent_classes_h
#define rootevent_classes_h
#include <string>
#include <utility>
#include <vector>
// Ensure dictionary is generated for vector<pair<string,string>>
// used by RootEventOutputer and RootBatchEventsOutputer Meta TTree branches.
typedef std::vector<std::pair<std::string, std::string>> VectorPairStringString;
#endif
