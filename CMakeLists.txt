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


#ROOT
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

#TBB
find_package(TBB REQUIRED)
list(APPEND EXTRA_LIBS tbb)
list(APPEND EXTRA_INCLUDES ${TBB_INCLUDE_DIR})

#LZ4
find_package(LZ4 REQUIRED)
list(APPEND EXTRA_LIBS lz4)
list(APPEND EXTRA_INCLUDES ${LZ4_INCLUDE_DIR})

#ZSTD
find_package(ZSTD REQUIRED)
list(APPEND EXTRA_LIBS zstd)
list(APPEND EXTRA_INCLUDES ${ZSTD_INCLUDE_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#make the library for testing
add_library(configKeys configKeyValuePairs.cc)

# add the executable
add_executable(threaded_io_test threaded_io_test.cc sourceFactoryGenerator.cc outputerFactoryGenerator.cc SerialTaskQueue.cc RootSource.cc PDSSource.cc PDSOutputer.cc RepeatingRootSource.cc Lane.cc RootOutputer.cc configKeyValuePairs.cc TBufferMergerRootOutputer.cc SerialRootSource.cc TextDumpOutputer.cc pds_reading.cc SharedPDSSource.cc TestProductsSource.cc TestProductsOutputer.cc)

target_include_directories(threaded_io_test PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           ${EXTRA_INCLUDES}
                           )

target_link_directories(threaded_io_test PUBLIC ${TBB_LIB_DIR})
target_link_directories(threaded_io_test PUBLIC ${LZ4_LIB_DIR})
target_link_directories(threaded_io_test PUBLIC ${ZSTD_LIB_DIR})
target_link_libraries(threaded_io_test PUBLIC ROOT::RIO ROOT::Tree ROOT::Core)
target_link_libraries(threaded_io_test PUBLIC ${EXTRA_LIBS})
target_link_libraries(threaded_io_test PRIVATE Threads::Threads)

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
add_test(NAME RootOutputerEmptyTest COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root)
add_test(NAME RootOutputerEmptySplitLevelTest COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root:splitLevel=1)
add_test(NAME RootOutputerEmptyAllOptionsTest COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root:splitLevel=1:compressionLevel=1:compressionAlgorithm=LZMA:basketSize=32000:treeMaxVirtualSize=-1:autoFlush=900)
add_test(NAME TestProductsROOT COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test SerialRootSource=test_prod.root 1 1 0 10 TestProductsOutputer")
add_test(NAME TestProductsROOTReplicated COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod_repl.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test ReplicatedRootSource=test_prod_repl.root 1 1 0 10 TestProductsOutputer")
add_test(NAME TestProductsROOTRepeating COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test TestProductsSource 1 1 0 10 RootOutputer=test_prod_rep.root; ${CMAKE_CURRENT_BINARY_DIR}/threaded_io_test RepeatingRootSource=test_prod_rep.root:5 1 1 0 100 TestProductsOutputer")
add_test(NAME TBufferMergerRootOutputerEmptyTest COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root)
add_test(NAME TBufferMergerRootOutputerEmptySplitLevelTest COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root:splitLevel=1)
add_test(NAME TBufferMergerRootOutputerEmptyAllOptionsTest COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root:splitLevel=1:compressionLevel=1:compressionAlgorithm=LZMA:basketSize=32000:treeMaxVirtualSize=-1:autoFlush=900)
add_test(NAME UseIMTTest COMMAND threaded_io_test EmptySource 1/useIMT 1 0 10)
