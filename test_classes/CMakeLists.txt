#ROOT
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

REFLEX_GENERATE_DICTIONARY(G__test_classes TestClasses.h SELECTION classes_def.xml)

add_library(test_classes_dict SHARED G__test_classes.cxx)
target_link_libraries(test_classes_dict PUBLIC ROOT::RIO ROOT::Net)
