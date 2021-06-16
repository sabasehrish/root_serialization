enable_testing()

add_executable(doTests test_main.cc test_configKeyValuePairs.cc)

target_include_directories(doTests PUBLIC ${PROJECT_BINARY_DIR}
                                          "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(doTests PRIVATE configKeys)

set(TEST_EXEC "${CMAKE_CURRENT_BINARY_DIR}/../threaded_io_test")

add_test(NAME RunTests COMMAND doTests)

add_test(NAME EmptySourceTest COMMAND threaded_io_test EmptySource 1 1 0 10)
add_test(NAME DummyOutputerTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 DummyOutputer)
add_test(NAME DummyOutputerUseProductReadyTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 DummyOutputer=useProductReady)
add_test(NAME TestProductsTest COMMAND threaded_io_test TestProductsSource 1 1 0 10 TestProductsOutputer)
add_test(NAME SerializeOutputerTest COMMAND threaded_io_test EmptySource 1 1 0 10 SerializeOutputer)
add_test(NAME SerializeOutputerVerboseTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 SerializeOutputer=verbose)
add_test(NAME PDSOutputerEmptyTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 PDSOutputer=test_empty.pds)
add_test(
  NAME TestProductsPDS
  COMMAND
    bash -c
    "${TEST_EXEC} TestProductsSource 1 1 0 10 PDSOutputer=test_prod.pds; ${TEST_EXEC} SharedPDSSource=test_prod.pds 1 1 0 10 TestProductsOutputer"
)
add_test(NAME RootOutputerEmptyTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root)
add_test(NAME RootOutputerEmptySplitLevelTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 RootOutputer=test_empty.root:splitLevel=1)
add_test(
  NAME RootOutputerEmptyAllOptionsTest
  COMMAND
    threaded_io_test EmptySource 1 1 0 10
    RootOutputer=test_empty.root:splitLevel=1:compressionLevel=1:compressionAlgorithm=LZMA:basketSize=32000:treeMaxVirtualSize=-1:autoFlush=900
)
add_test(
  NAME TestProductsROOT
  COMMAND
    bash -c
    "${TEST_EXEC} TestProductsSource 1 1 0 10 RootOutputer=test_prod.root; ${TEST_EXEC} SerialRootSource=test_prod.root 1 1 0 10 TestProductsOutputer"
)
add_test(
  NAME TestProductsROOTReplicated
  COMMAND
    bash -c
    "${TEST_EXEC} TestProductsSource 1 1 0 10 RootOutputer=test_prod_repl.root; ${TEST_EXEC} ReplicatedRootSource=test_prod_repl.root 1 1 0 10 TestProductsOutputer"
)
add_test(
  NAME TestProductsROOTRepeating
  COMMAND
    bash -c
    "${TEST_EXEC} TestProductsSource 1 1 0 10 RootOutputer=test_prod_rep.root; ${TEST_EXEC} RepeatingRootSource=test_prod_rep.root:5 1 1 0 100 TestProductsOutputer"
)
add_test(NAME TBufferMergerRootOutputerEmptyTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root)
add_test(NAME TBufferMergerRootOutputerEmptySplitLevelTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 TBufferMergerRootOutputer=test_empty.root:splitLevel=1)
add_test(
  NAME TBufferMergerRootOutputerEmptyAllOptionsTest
  COMMAND
    threaded_io_test EmptySource 1 1 0 10
    TBufferMergerRootOutputer=test_empty.root:splitLevel=1:compressionLevel=1:compressionAlgorithm=LZMA:basketSize=32000:treeMaxVirtualSize=-1:autoFlush=900
)
add_test(NAME UseIMTTest COMMAND threaded_io_test EmptySource 1/useIMT 1 0 10)
add_test(NAME HDFOutputerEmptyTest
         COMMAND threaded_io_test EmptySource 1 1 0 10 HDFOutputer=test_empty.h5)
add_test(
  NAME TestProductsHDF
  COMMAND
    bash -c
    "${TEST_EXEC} TestProductsSource 1 1 0 10 HDFOutputer=test_prod.h5; ${TEST_EXEC} HDFSource=test_prod.h5 1 1 0 10 TestProductsOutputer"
)
