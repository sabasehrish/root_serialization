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
list(APPEND EXTRA_LIBS Core)
list(APPEND EXTRA_LIBS RIO)
list(APPEND EXTRA_LIBS Tree)
list(APPEND EXTRA_INCLUDES "/cvmfs/cms.cern.ch/slc7_amd64_gcc820/lcg/root/6.20.06-ghbfee5/include")


#TBB
list(APPEND EXTRA_LIBS tbb)
list(APPEND EXTRA_INCLUDES "/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/tbb/2020_U2-ghbfee/include")

#LZ4
list(APPEND EXTRA_LIBS lz4)
list(APPEND EXTRA_INCLUDES "/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/lz4/1.9.2/include")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# add the executable
add_executable(threaded_io_test threaded_io_test.cc sourceFactoryGenerator.cc outputerFactoryGenerator.cc SerialTaskQueue.cc RootSource.cc PDSSource.cc)

target_include_directories(threaded_io_test PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           ${EXTRA_INCLUDES}
                           )

target_link_directories(threaded_io_test PUBLIC "/cvmfs/cms.cern.ch/slc7_amd64_gcc820/lcg/root/6.20.06-ghbfee5/lib")
target_link_directories(threaded_io_test PUBLIC "/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/tbb/2020_U2-ghbfee/lib")
target_link_directories(threaded_io_test PUBLIC "/cvmfs/cms.cern.ch/slc7_amd64_gcc820/external/lz4/1.9.2/lib")

target_link_libraries(threaded_io_test PUBLIC ${EXTRA_LIBS})
target_link_libraries(threaded_io_test PRIVATE Threads::Threads)
