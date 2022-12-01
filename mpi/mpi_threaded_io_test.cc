#include "TROOT.h"
#include "TVirtualStreamerInfo.h"
#include "TObject.h"
#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <string>
#include <atomic>
#include <iomanip>
#include <cmath>

#include "../outputerFactoryGenerator.h"
#include "../sourceFactoryGenerator.h"
#include "../waiterFactoryGenerator.h"

#include "../Lane.h"

#include "tbb/task_group.h"
#include "tbb/global_control.h"
#include "tbb/task_arena.h"

#include "CLI11.hpp"
#include "mpicpp.h"


namespace {
  std::vector<std::string> fnames(std::string flist) {
    std::ifstream is(flist);
    std::istream_iterator<std::string> i_start(is), i_end;
    return std::vector<std::string>(i_start, i_end);
  }   

  
  std::string ifile(int mode, int rank, std::vector<std::string> ilist) {
    if (ilist.empty()) return "";
    if (mode == 0) return ilist[rank];
    if (mode == 1) return ilist[rank];
    if (mode == 2) return ilist[0];
    return ilist[0];
  }
  
  std::string ofile(int mode, int rank, std::vector<std::string> olist) {
    if (mode == 0) return olist[rank];
    if (mode == 1) return olist[0];
    if (mode == 2) return olist[0];
    return olist[rank];
  }
 
   std::pair<std::string, std::string> parseCompound(std::string_view iArg) {
     std::string sArg(iArg);
       auto foundEq = sArg.find('=');
       auto foundComma = sArg.find(':');
       auto found = foundEq < foundComma ? foundEq : foundComma;
       if(found != std::string::npos) {
         return std::pair(sArg.substr(0,found), sArg.substr(found+1));
       }
       return std::pair(sArg, std::string());
   }

   int batchSize(std::string s) {
   // example is:  filename:batchSize=15:chunkSize=1024
    std::string delimiter = ":";
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        if (token.find("batchSize=") != std::string::npos){
          auto bs = token.substr(10, token.length()-1);
          return std::stoi(bs);
        }
        s.erase(0, pos + delimiter.length());
    }
    return 0; 
   }

  bool same_num_of_batches(int batch_size, int nranks, unsigned long long nEvents) {
    auto q = nEvents/nranks;
    auto rem = nEvents%nranks;
    std::vector<int> event_ranges;
    event_ranges.reserve(nranks);
    auto start = 0;
    auto end = 0;
    for(auto i=0; i< nranks;++i) {
      end = start + q + static_cast<int>(i<rem);
      if (i == nranks-1) end = nEvents;
      event_ranges.emplace_back((end-start)/batch_size + static_cast<int>(((end-start)%batch_size)!=0));
      start = end;
    }

    auto result = std::adjacent_find( event_ranges.begin(), event_ranges.end(), std::not_equal_to<>() ) == event_ranges.end();
    return result; 
  }

  std::vector<std::pair<long, long>> ranges(int mode, int nranks, int batch_size, unsigned long long nEvents) {
    auto rem = nEvents%nranks;
    if (!same_num_of_batches(batch_size, nranks, nEvents)) {
        nEvents = nEvents + nranks - rem;
        std::cout << "batch sizes not same!" << std::endl;
    }
    auto q = nEvents/nranks;
    rem = nEvents%nranks;
    std::vector<std::pair<long, long>> event_ranges;
    event_ranges.reserve(nranks);
    auto start = 0;
    auto end = 0;
    for(auto i=0; i< nranks;++i) {
      end = start + q + static_cast<int>(i<rem);
      if (i == nranks-1) end = nEvents;
      event_ranges.emplace_back(start, end);
      start = end;
    }
    return event_ranges;
  }
}

int main(int argc, char* argv[]) {
  Mpi mpi(argc, argv); 
  auto const my_rank =  mpi.rank();
  auto const nranks = mpi.np();
  using namespace cce::tf;

  CLI::App app{"Mpi_threaded_IO"};
  int mode; 
  // assuming n ranks, here are different mode values and their meanings
  // 0: n-n, i.e. n ranks reading n files and writing n files (1 input/output file per rank), should work for all output types
  // 1: n-1, i.e. n ranks reading n files and writing 1 file (1 input file per rank, 1 output file for all ranks), valid only for writig HDF5 output
  // 2: 1-1, valid for HDF5 output only in case of n ranks, and for all output formats if there is only 1 rank. 
  // 3: 1-n , should work for all output types
  std::string isource;
  std::string ilist;
  int jt;
  bool useIMT;
  int je;
  double scale;
  int nevents;
  std::string outputType;
  std::string olist;

  app.add_option("-m, --mode", mode, "Operation mode")->required();
  app.add_option("-s, --source", isource, "Input Source")->required();
  app.add_option("--ilist", ilist, "Name of file with a listing of input file names to be read");
  app.add_option("--jt", jt, "Number of threads")->required();
  app.add_option("--useIMT", useIMT, "use IMT")->required();
  app.add_option("--je", je, "Number of events per thread")->required();
  app.add_option("--scale", scale, "Scale")->required();
  app.add_option("-n, --nevents", nevents, "Total number of events to process across all MPI ranks")->required();
  app.add_option("-o, --outputtype", outputType, "Output Source")->required();
  app.add_option("--olist", olist, "Name of file with a listing of output file names to be generated")->required();
  std::string waiterConfig;
  app.add_option("-w,--waiter", waiterConfig, "configure Waiter.\nDefault is no waiter denoted by ''.");

  app.parse(argc, argv);
  
  int parallelism = tbb::this_task_arena::max_concurrency();
  tbb::global_control c(tbb::global_control::max_allowed_parallelism, parallelism);

  //Tell Root we want to be multi-threaded
  if(useIMT) {
    ROOT::EnableImplicitMT(parallelism);
  } else {
    ROOT::EnableThreadSafety();
  }
  //When threading, also have to keep ROOT from logging all TObjects into a list
  TObject::SetObjectStat(false);
  
  //Have to avoid having Streamers modify themselves after they have been used
  TVirtualStreamerInfo::Optimize(false);

  std::vector<Lane> lanes;
  unsigned int nLanes = je;

  unsigned long long nEvents = nevents; 

  std::vector<std::string> outputInfo = fnames(olist);
  std::vector<std::string> sourceOptions = fnames(ilist);
  auto ifilename = ifile(mode, my_rank, sourceOptions);
  auto ofilename = ofile(mode, my_rank, outputInfo);

  if (outputType == "TextDumpOutputer") ofilename = "";
  auto outFactory = outputerFactoryGenerator(outputType, ofilename); 
  auto sourceFactory = sourceFactoryGenerator(isource, ifilename);
  auto batch_size_per_rank = batchSize(ofilename);
  //std::cout << "Batch size: " << batch_size_per_rank << std::endl;

  if (not sourceFactory) {
    std::cout <<"unknown source type "<<isource<<std::endl;
    return 1;
  }
 
 // checking operation modes to make sure we only run valid combos
 //
  switch (mode) {
  case 0:
   if (sourceOptions.size() != outputInfo.size() ) {
    std::cout << "Number of Input and output files dont match\n";
    return 1;
  } else {
    if (sourceOptions.size() != nranks) {
      std::cout << "Number of ranks dont match input/output files\n";
      return 1;
    }
  }
  break;
  case 1:
    //only supported for HDFOutputer
    if (outputType != "PHDFBatchEventsOutputer" && outputType != "PHDFBatchEventsV2Outputer") {
      std::cerr << "Writing to a single file is only supported for HDF5\n";
      return 1;
    }
    break;
  case 2:
    //only supported for HDFOutputer
    //std::cout << outputType << std::endl;
    if (outputType != "PHDFBatchEventsOutputer" && outputType != "PHDFBatchEventsV2Outputer") {
      std::cerr << "Writing to a single file is only supported for HDF5\n";
      return 1;
    }
    break;
  default:
  break;
}
  
  decltype(waiterFactoryGenerator(waiterConfig, waiterConfig)) waiterFactory;
  if(not waiterConfig.empty()) {
    auto [type, options] = parseCompound(waiterConfig);
    waiterFactory = waiterFactoryGenerator(type, options);
    if(not waiterFactory) {
      std::cout <<"unknown waiter type "<<type<<std::endl;
      return 1;
    }
  }

  std::cout << my_rank <<" begin warmup"<<std::endl;
  {
    //warm up the system by processing 1 event 
    tbb::task_arena arena(1);
    auto out = outFactory(1, 1);
    auto source =sourceFactory(1,1);
    Lane lane(0, source.get(), nullptr);
    out->setupForLane(0, lane.dataProducts());
    auto pOut = out.get();
    arena.execute([&lane,pOut]() {
        tbb::task_group group;
        std::atomic<long> ievt{0};
	std::atomic<unsigned int> count{0};
        group.run([&]() {
            lane.processEventsAsync(ievt, group, *pOut, AtomicRefCounter(count));
          });
        group.wait();
      });
  }
  std::cout <<"finished warmup"<<std::endl;
  
  //calculate first event index and last event index for this rank
  //only for PHDFBatchEventsOutputer, should clean up later
  int firsteventIndex;
  int lasteventIndex;
  auto event_ranges = ranges(mode, nranks, batch_size_per_rank, nEvents);
  std::tie(firsteventIndex, lasteventIndex) = event_ranges[my_rank];
  auto localevents = (my_rank == nranks-1) ? (nEvents - firsteventIndex) : (lasteventIndex - firsteventIndex);
  auto out = outFactory(nLanes, localevents);
  auto source = sourceFactory(nLanes, lasteventIndex);
 
  if (outputType == "PHDFBatchEventsV2Outputer") {
    out = outFactory(nLanes, 0);
    source = sourceFactory(nLanes, nEvents);
  }
  std::unique_ptr<WaiterBase> waiter;
  if(waiterFactory) {
    waiter = waiterFactory(nLanes, source->numberOfDataProducts());
    if(not waiter) {
      std::cout <<"failed to create Waiter "<<waiterConfig<<std::endl;
      return 1;
    }
  }

  lanes.reserve(nLanes);
  for(unsigned int i = 0; i< nLanes; ++i) {
    lanes.emplace_back(i, source.get(), waiter.get());
    out->setupForLane(i, lanes.back().dataProducts());
  }
  
  std::atomic<long> ievt = 0; 
  if (outputType == "PHDFBatchEventsOutputer"){ 
    ievt = firsteventIndex;
  } else firsteventIndex = 0; 

  tbb::task_arena arena(parallelism);

  decltype(std::chrono::high_resolution_clock::now()) start;
  auto pOut = out.get();
  arena.execute([&lanes, &ievt, pOut, &start, my_rank, nranks]() {
    std::atomic<unsigned int> nLanesWaiting{ 0 };
    std::vector<tbb::task_group> groups(lanes.size());
    start = std::chrono::high_resolution_clock::now();
    auto itGroup = groups.begin();
    {
      AtomicRefCounter laneCounter(nLanesWaiting);
      for(auto& lane: lanes) {
        auto& group = *itGroup;
        group.run([&]() {lane.processEventsAsync(ievt,group, *pOut,laneCounter);});
        ++itGroup;
      }
    }
    do {
      for(auto& group: groups) {
	    group.wait();
      }
    } while(nLanesWaiting != 0);
    //be sure all groups have fully finished
    for(auto& group: groups) {
      group.wait();
    }
  });
  std::string timingfile = "Timing_"+std::to_string(my_rank)+".txt";
  std::ofstream fout(timingfile);
  fout << std::setprecision(4);
  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);
  //NOTE: each lane will go 1 beyond the # events so ievt is more then the # events
  fout <<"MPI threaded_io_test stats"<<std::endl;
  fout <<"Source: "<< isource<<"\n"
            <<"Outputer: "<< outputType<<"\n"
	    <<"Num threads: "<<parallelism<<"\n"
	    <<"Num concurrent events: "<<nLanes <<"\n"
	    <<"Time scale: "<<scale<<"\n"
	    <<"Use ROOT IMT: "<< (useIMT? "true\n":"false\n");
  fout <<"Event processing time for rank " << my_rank << ": " << eventTime.count()<<"us"<<std::endl;
  // Each lane will increment the value first and then check if it can process that 
  // event or not so we need to subtract nLanes 
  fout <<"Number events read by rank "<< my_rank<< ": " << ievt.load() - firsteventIndex - nLanes << std::endl;
  fout <<"----------"<<std::endl;

  source->printSummary();
  out->printSummary();
  //fout << my_rank <<" finished processing\n";
  return 0;
}
