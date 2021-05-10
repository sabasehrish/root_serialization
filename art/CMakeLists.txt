find_package(OpenSSL REQUIRED COMPONENTS Crypto)

add_library(canvas_standalone
  SHARED
    EventAuxiliary.cc
    Exception.cc
    EventID.cc
    Hash.cc
    MD5Digest.cc
    nybbler.cc
    RunID.cc
    SubRunID.cc
    exception.cc)
target_link_libraries(canvas_standalone PRIVATE OpenSSL::Crypto)
