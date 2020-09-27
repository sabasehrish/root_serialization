# root_serialization

This is a testing structure for doing performance experiments with various I/O packages from within an multi-threaded program. The program mimics behaviors common for
HEP data processing frameworks.

## Getting started
The CMakeLists.txt uses 3rd party packages from /cvmfs which are identical to the packages used by the CMS experiment's CMSSW_11_2_0_pre6 software release. No CMS
code is needed to build the code from this repository. CMS header files and shared libraries are needed if data files containing CMS data are being read.

To build
1. git clone ....
1. cd root_serialization
1. mkdir build
1. cd build
1. cmake ../
1. make

This will create the executable `threaded_io_test`.

## threaded_io_test design
The testing structure has 2 customizable component types
1. `Source`s: These supply the _event_ data products used for testing.
1. `Outputer`s: These read the _event_ data products. Some Outputers also write that data out for storage.

In order to mimic the time taken to process event data, the testing structure has `Waiter`s. A `Waiter` read one _event_ data product and based on a property of that data
product calls sleep for a length proportional to that property.

The testing structure can process multiple _events_ concurrently. Each concurrent _event_ has its own set of `Waiter`s, one for each data product. The processing of 
an _event_ is handled by a `Lane`. Concurrent _events_ are then done by having multiple `Lane`s. The `Waiter`s within a `Lane` are considered to be completely 
independent and can be run concurrently.

All `Lane`s share the same `Outputer`. Therefore an `Outputer` is required to be thread safe.

For the moment, each `Lane` also has its own copy of the `Source`. That is likely to be changed in the future to better mimic the behavior of actual HEP processing
framemworks.

The want data is processed is as follows:
1. When a `Lane` is no longer processing an _event_ it requests a new one from the system. The system advances an atomic counter and tells the `Lane` to use
the _event_ associated with that index.
1. The `Lane` then passes the _event_ index to the `Source` and asks it to asynchronously retrieve the _event_ data products.
1. Once the `Source` has retrieved the data products, it signals to the `Waiter`s to run asynchronously.
1. Once each `Waiter` has finished, the system signals to the `Outputer` that the particular _event_ data product is available for the `Outputer`. The `Outputer`
can then asynchronously process that data product.
1. Once the `Outputer` has finished with all the data products, the system signals the `Outputer` that the _event_ has finished. The `Outputer` can then do
additional asynchronous work.
1. When the `Outputer` finishes its end of _event_ work, the `Lane` is considered to be done with that _event_ and the cycle repeats.

## Running tests
The `threaded_io_test` takes up to 5 command line arguments
```
threaded_io_test <Source configuration> [# threads] [# conconcurrent events] [time scale factor] [max # events] [<Outputer configuration>]
```

1. `<Source configuration>` : which `Source` to use and any additional information needed to configure it. Options are described below.
1. `[# threads]` : number of threads to use in the job.
1. `[# concurrent events]` : number of _events_ (that is `Lane`s) to use. Best if number of events is less than  or equal to number of threads.
1. `[time scale factor]` : used to convert the property of the _event_ data products into microseconds used for the sleep call. A value of 0 means no sleeping. A value less than 0 prohibits the creation of the objects which do the sleep.
1. `[<Outputer configuration>]` : optional, used to specify which `Outputer` to use and any additional information needed to configure it. The exact options are
described below.

## Available Components

### Sources

#### EmptySource
Does not generate any _event_ data products. Specify by just using its name, e.g. 
```
> threaded_io_test EmptySource 1 1 0 10
```
#### ReplicatedRootSource
Reads a standard ROOT file. Each concurrent Event has its own replica of the Source to avoid the need for cross Event synchronization. In addition to its name, one needs to give the file to read, e.g.
```
> threaded_io_test ReplicatedRootSource=test.root 1 1 0 10
```

#### RepeatingRootSource
Reads the first N events from a standard ROOT file at construction time. The deserialized data products are held in memory. Going from event to event is just a switch of the memory addresses to be used. In addition to its name, one needs to give the file to read and, optionally, the number of events to read (default is 10) , e.g.

```
> threaded_io_test RepeatingRootSource=test.root 1 1 0 1000
```
or
```
> threaded_io_test RepeatingRootSource=test.root:5 1 1 0 1000
```


#### ReplicatedPDSSource
Reads a _packed data streams_ format file. Each concurrent Event has its own replica of the Source to avoid the need for cross Event synchronization. In addition to its name, one needs to give the file to read, e.g.
```
> threaded_io_test ReplicatedPDSSource=test.pds 1 1 0 10
```

### Outputers

#### DummyOutputer
Does no work. If no outputer is given, this is the one used. Specify by just using its name and optionally
the option label 'useProductReady'
```
> threaded_io_test EmptySource 1 1 0 10 DummyOutputer
```
or
```
> threaded_io_test EmptySource 1 1 0 10 DummyOutputer=useProductReady
```


#### SerializeOutputer
Uses ROOT to serialize the _event_ data products but does not store them. It prints timing statistics about the serialization. Specify by just using its name and an optional 'verbose' parameter
```
> threaded_io_test ReplicatedRootSource=test.root 1 1 0 10 SerializeOutputer
```
or
```
> threaded_io_test ReplicatedRootSource=test.root 1 1 0 10 SerializeOutputer=verbose
```

#### RootOutputer
Writes the _event_ data products into a ROOT file. Specify both the name of the Outputer and the file to write as well as many  optional parameters:
- splitLevel: split level for the branches, default 99
- compressionLevel: compression level 0-9, default 9
- compressionAlgorithm: name of compression algorithm. Allowed valued "", "ZLIB", "LZMA", "LZ4"
- basketSize: default size of all baskets, default size 16384
- treeMaxVirtualSize: Size of ROOT TTree TBasket cache. Use ROOT default if value is <0. Default -1.
- useIMT : if specified, uses ROOT's Implicit Multi-threading ability
```
> threaded_io_test ReplicatedRootSource=test.root 1 1 0 10 RootOutputer=test.root
```
or
```
> threaded_io_test ReplicatedRootSource=test.root 1 1 0 10 RootOutputer=test.root:splitLevel=1
```


#### PDSOutputer
Writes the _event_ data products into a PDS file. Specify both the name of the Outputer and the file to write
```
> threaded_io_test ReplicatedRootSource=test.root 1 1 0 10 PDSOutputer=test.pds
```


