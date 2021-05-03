#ROOT
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

REFLEX_GENERATE_DICTIONARY(G__cms EventAuxiliary.h EventID.h HashedTypes.h Hash.h ProcessHistoryID.h RunID.h RunLumiEventNumber.h Timestamp.h SELECTION classes_def.xml)

add_library(cms_dict SHARED G__cms.cxx)
target_link_libraries(cms_dict PUBLIC ROOT::RIO ROOT::Net)
