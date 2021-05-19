# ============================================================================
# Configuration instructions
#
# cmake <dir. with this file> \
#   -DROOT_DIR=$ROOTSYS/cmake \
#   -DHIGHFIVE_INC=<include directory of HighFive> \
#   -Dzstd_DIR=<path to zstd lib>/cmake/zstd

cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# set the project name
project(threaded_io_test VERSION 0.1)

# want debug symbols
set(CMAKE_CXX_FLAGS "-g")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(ThreadedIOTestConfig.h.in ThreadedIOTestConfig.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(DEFINED zstd_DIR)
  find_package(zstd REQUIRED)
  list(APPEND EXTRA_LIBS zstd::libzstd_shared)
else()
  list(APPEND EXTRA_LIBS zstd)
endif()

find_package(ROOT REQUIRED COMPONENTS)

list(APPEND EXTRA_LIBS "tbb;lz4")

add_library(configKeys configKeyValuePairs.cc)

# add the executable
add_executable(
  threaded_io_test
  HDFOutputer.cc
  HDFSource.cc
  Lane.cc
  PDSOutputer.cc
  PDSSource.cc
  RepeatingRootSource.cc
  RootOutputer.cc
  RootSource.cc
  SerialRootSource.cc
  SerialTaskQueue.cc
  SharedPDSSource.cc
  TBufferMergerRootOutputer.cc
  TestProductsOutputer.cc
  TestProductsSource.cc
  TextDumpOutputer.cc
  configKeyValuePairs.cc
  outputerFactoryGenerator.cc
  pds_reading.cc
  sourceFactoryGenerator.cc
  threaded_io_test.cc)

target_include_directories(
  threaded_io_test PRIVATE ${PROJECT_BINARY_DIR} ${EXTRA_INCLUDES}
  ${HIGHFIVE_INC} $ENV{HDF5_INC} $ENV{TBB_INC})

target_link_directories(threaded_io_test PRIVATE $ENV{TBB_LIB} $ENV{HDF5_LIB})
target_link_libraries(threaded_io_test PRIVATE Threads::Threads ROOT::RIO
                                               ROOT::Tree hdf5 ${EXTRA_LIBS})

enable_testing()
add_subdirectory(tests)
