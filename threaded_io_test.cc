#include "TROOT.h"
#include "TVirtualStreamerInfo.h"
#include "TObject.h"
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <iomanip>

#include "outputerFactoryGenerator.h"
#include "sourceFactoryGenerator.h"

#include "Lane.h"

#include "tbb/task_group.h"
#include "tbb/global_control.h"
#include "tbb/task_arena.h"

namespace {
  std::pair<std::string, std::string> parseCompound(const char* iArg) {
    std::string sArg(iArg);
    auto found = sArg.find('=');
    auto next = found;
    if(found != std::string::npos) {
      return std::pair(sArg.substr(0,found), sArg.substr(found+1));
    }
    return std::pair(sArg, std::string());
  }
}

int main(int argc, char* argv[]) {
  using namespace cce::tf;

  if(not (argc > 1 and argc < 8) ) {
    std::cout <<"1 to 6 arguments required\n"
                "cms_read_threaded <filename> [# threads] [# conconcurrent events] [time scale factor] [max # events] [<output file name>]\n";
    return 1;
  }

  //Tell Root we want to be multi-threaded
  ROOT::EnableThreadSafety();
  
  //When threading, also have to keep ROOT from logging all TObjects into a list
  TObject::SetObjectStat(false);
  
  //Have to avoid having Streamers modify themselves after they have been used
  TVirtualStreamerInfo::Optimize(false);

  size_t parallelism = tbb::this_task_arena::max_concurrency();
  if(argc > 2) {
    parallelism = atoi(argv[2]);
  }
  tbb::global_control c(tbb::global_control::max_allowed_parallelism, parallelism);

  std::vector<Lane> lanes;
  unsigned int nLanes = 4;
  if(argc > 3) {
    nLanes = atoi(argv[3]);
  }

  double scale = 0.;
  if(argc > 4) {
    scale = atof(argv[4]);
  }

  unsigned long long nEvents = std::numeric_limits<unsigned long long>::max();
  if(argc > 5) {
    nEvents = atoi(argv[5]);
  }


  std::function<std::unique_ptr<OutputerBase>(unsigned int)> outFactory;
  std::string outputerName = "DummyOutputer";
  if(argc == 7) {
    outputerName = argv[6];
    auto [outputType, outputInfo] = parseCompound(argv[6]);
    outFactory = outputerFactoryGenerator(outputType, outputInfo);
    if(not outFactory) {
      std::cout <<"unknown output type "<<outputType<<std::endl;
      return 1;
    }
  } else {
    outFactory = outputerFactoryGenerator(outputerName, "");
  }

  auto [sourceType, sourceOptions] = parseCompound(argv[1]);
  auto sourceFactory = sourceFactoryGenerator(sourceType, sourceOptions);
  if(not sourceFactory) {
    std::cout <<"unknown source type "<<sourceType<<std::endl;
    return 1;
  }

  std::cout <<"begin warmup"<<std::endl;
  {
    //warm up the system by processing 1 event 
    tbb::task_arena arena(1);
    auto out = outFactory(1);
    Lane lane(0, sourceFactory(1), 0);
    out->setupForLane(0, lane.dataProducts());
    auto pOut = out.get();
    arena.execute([&lane,pOut]() {
        tbb::task_group group;
        std::atomic<long> ievt{0};
        group.run([&]() {
            lane.processEventsAsync(ievt, group, *pOut);
          });
        group.wait();
      });
  }
  std::cout <<"finished warmup"<<std::endl;

  auto out = outFactory(nLanes);
  lanes.reserve(nLanes);
  for(unsigned int i = 0; i< nLanes; ++i) {
    lanes.emplace_back(i, sourceFactory(nEvents), scale);
    out->setupForLane(i, lanes.back().dataProducts());
  }

  std::atomic<long> ievt{0};
  
  tbb::task_arena arena(parallelism);

  decltype(std::chrono::high_resolution_clock::now()) start;
  auto pOut = out.get();
  arena.execute([&lanes, &ievt, pOut, &start]() {
    tbb::task_group group;
    start = std::chrono::high_resolution_clock::now();
    group.run([&]() {
        for(auto& lane : lanes) {
          lane.processEventsAsync(ievt, group, *pOut);
        }
      });
    
    group.wait();
  });

  std::chrono::microseconds eventTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-start);

  //NOTE: each lane will go 1 beyond the # events so ievt is more then the # events
  std::cout <<"----------"<<std::endl;
  std::cout <<"Source "<<argv[1]<<"\n"
            <<"Outputer "<<outputerName<<"\n"
	    <<"# threads "<<parallelism<<"\n"
	    <<"# concurrent events "<<nLanes <<"\n"
	    <<"time scale "<<scale<<"\n";
  std::cout <<"Event processing time: "<<eventTime.count()<<"us"<<std::endl;
  std::cout <<"number events: "<<ievt.load() -nLanes<<std::endl;
  std::cout <<"----------"<<std::endl;

  std::chrono::microseconds sourceTime = std::chrono::microseconds::zero();
  for(auto const& lane: lanes) {
    sourceTime += lane.sourceAccumulatedTime();
  }

  std::cout <<"\nSource time: "<<sourceTime.count()<<"us\n"<<std::endl;

  out->printSummary();
}
