cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# set the project name
project(threaded_io_test VERSION 0.1 LANGUAGES CXX)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

# want debug symbols
set(CMAKE_CXX_FLAGS "-g3")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)
find_package(TBB REQUIRED)
find_package(zstd REQUIRED)
find_package(lz4 REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

configure_file(ThreadedIOTestConfig.h.in ThreadedIOTestConfig.h)

#make the library for testing
add_library(configKeys configKeyValuePairs.cc)

#make the library holding the root dictionaries
REFLEX_GENERATE_DICTIONARY(G__sequence_classes SequenceFinderForBuiltins.h SELECTION classes_def.xml)

add_library(sequence_classes_dict SHARED G__sequence_classes.cxx)
target_link_libraries(sequence_classes_dict PUBLIC ROOT::RIO ROOT::Net)

add_executable(threaded_io_test
  DeserializeStrategy.cc
  Lane.cc
  PDSOutputer.cc
  PDSSource.cc
  RepeatingRootSource.cc
  RootOutputer.cc
  RootSource.cc
  SerialRootSource.cc
  SerialTaskQueue.cc
  SerializeStrategy.cc
  SharedPDSSource.cc
  TBufferMergerRootOutputer.cc
  TestProductsOutputer.cc
  TestProductsSource.cc
  TextDumpOutputer.cc
  UnrolledDeserializer.cc
  UnrolledSerializer.cc
  common_unrolling.cc
  outputerFactoryGenerator.cc
  pds_reading.cc
  sourceFactoryGenerator.cc
  threaded_io_test.cc)

target_include_directories(threaded_io_test PRIVATE "${PROJECT_BINARY_DIR}")

target_link_libraries(threaded_io_test
                      PRIVATE LZ4::lz4
                              ROOT::Core
                              ROOT::RIO
                              ROOT::Tree
                              TBB::tbb
                              Threads::Threads
                              configKeys
                              sequence_classes_dict
                              zstd::libzstd_shared)

add_subdirectory(cms)
add_subdirectory(test_classes)

add_executable(unroll_test 
  UnrolledDeserializer.cc 
  UnrolledSerializer.cc
  common_unrolling.cc
  unroll_test.cc)

target_link_libraries(unroll_test
                      PRIVATE ROOT::Core
                              ROOT::RIO
                              ROOT::Tree
                              TBB::tbb
                              cms_dict
                              sequence_classes_dict
                              test_classes_dict)

enable_testing()
add_subdirectory(tests)
add_test(NAME EmptySourceTest COMMAND threaded_io_test EmptySource 1 1 0 10)
add_test(NAME DummyOutputerTest COMMAND threaded_io_test EmptySource 1 1 0 10 DummyOutputer)
add_test(NAME DummyOutputerUseProductReadyTest COMMAND threaded_io_test EmptySource 1 1 0 10 DummyOutputer=useProductReady)
add_test(NAME TextDumpOutputerPerEventTest COMMAND threaded_io_test EmptySource 1 1 0 10 TextDumpOutputer=perEvent=t)
add_test(NAME TextDumpOutputerSummaryTest COMMAND threaded_io_test EmptySource 1 1 0 10 TextDumpOutputer=summary=t)
add_test(NAME TestProductsTest COMMAND threaded_io_test TestProductsSource 1 1 0 10 TestProductsOutputer)
add_test(NAME SerializeOutputerTest COMMAND threaded_io_test EmptySource 1 1 0 10 SerializeOutputer)
add_test(NAME SerializeOutputerVerboseTest COMMAND threaded_io_test EmptySource 1 1 0 10 SerializeOutputer=verbose)
add_test(NAME PDSOutputerEmptyTest COMMAND threaded_io_test EmptySource 1 1 0 10 PDSOutputer=test_empty.pds)
add_test(NAME TestProductsPDS COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 PDSOutputer=test_prod.pds; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test SharedPDSSource=test_prod.pds 1 1 0 10 TestProductsOutputer")
add_test(NAME TestProductsPDSUnrolled COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 PDSOutputer=test_prod_unroll.pds:serializationAlgorithm=Unrolled; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test SharedPDSSource=test_prod_unroll.pds 1 1 0 10 TestProductsOutputer")
add_test(NAME RootOutputerEmptyTest COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root)
add_test(NAME RootOutputerEmptySplitLevelTest COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root:splitLevel=1)
add_test(NAME RootOutputerEmptyAllOptionsTest COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root:splitLevel=1:compressionLevel=1:compressionAlgorithm=LZMA:basketSize=32000:treeMaxVirtualSize=-1:autoFlush=900)
add_test(NAME TestProductsROOT COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test SerialRootSource=test_prod.root 1 1 0 10 TestProductsOutputer")
add_test(NAME TestProductsROOTReplicated COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod_repl.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test ReplicatedRootSource=test_prod_repl.root 1 1 0 10 TestProductsOutputer")
add_test(NAME TestProductsROOTRepeating COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod_rep.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test RepeatingRootSource=test_prod_rep.root:5 1 1 0 100 TestProductsOutputer")
add_test(NAME TestProductsROOTRepeatingOneBranch COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod_rep_1branch.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test RepeatingRootSource=test_prod_rep_1branch.root:5:floats 1 1 0 100 TestProductsOutputer")
add_test(NAME TBufferMergerRootOutputerEmptyTest COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root)
add_test(NAME TBufferMergerRootOutputerEmptySplitLevelTest COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root:splitLevel=1)
add_test(NAME TBufferMergerRootOutputerEmptyAllOptionsTest COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root:splitLevel=1:compressionLevel=1:compressionAlgorithm=LZMA:basketSize=32000:treeMaxVirtualSize=-1:autoFlush=900)
add_test(NAME UseIMTTest COMMAND threaded_io_test EmptySource 1/useIMT 1 0 10)
