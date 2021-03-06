if (NOT lz4_FOUND)
  if (lz4_FIND_REQUIRED)
    set(_lz4_FIND_REQUIRED ${lz4_FIND_REQUIRED})
    unset(lz4_FIND_REQUIRED)
  else()
    unset(_lz4_FIND_REQUIRED)
  endif()
  find_package(lz4 CONFIG QUIET)
  if (_lz4_FIND_REQUIRED)
    set(lz4_FIND_REQUIRED ${_lz4_FIND_REQUIRED})
    unset(_lz4_FIND_REQUIRED)
  endif()
  if (lz4_FOUND)
    set(_lz4_config_mode CONFIG_MODE)
  else()
    unset(_lz4_config_mode)
  endif()
endif()

if (NOT LZ4::lz4)
  if (NOT lz4_FOUND)
    find_library(_lz4_lib lz4 HINTS ${lz4_DIR})
    find_file(_lz4_h lz4.h HINTS ${lz4_DIR}/include)
    get_filename_component(_lz4_h_path ${_lz4_h} DIRECTORY CACHE)
    if (_lz4_lib AND _lz4_h)
      set(lz4_FOUND TRUE)
      set(LZ4_FOUND TRUE)
    endif()
  endif()
  add_library(LZ4::lz4 SHARED IMPORTED)
  set_target_properties(LZ4::lz4 PROPERTIES IMPORTED_LOCATION ${_lz4_lib})
  set_target_properties(LZ4::lz4 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${_lz4_h_path})
endif()

set(lz4_FIND_REQUIRED ${_lz4_FIND_REQUIRED})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lz4 ${_lz4_config_mode} REQUIRED_VARS lz4_FOUND)

unset(_lz4_FIND_REQUIRED)
unset(_lz4_lib CACHE)
unset(_lz4_h CACHE)
unset(_lz4_h_path CACHE)
unset(_lz4_config_mode)
