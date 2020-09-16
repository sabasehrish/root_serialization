#include "TROOT.h"
#include "TVirtualStreamerInfo.h"
#include "TObject.h"
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <iomanip>

#include "Outputer.h"
#include "PDSOutputer.h"
#include "PDSSource.h"
#include "RootSource.h"
#include "Lane.h"

#include "tbb/task_group.h"
#include "tbb/global_control.h"
#include "tbb/task_arena.h"

#include "SerialTaskQueue.h"
#include "SerialTaskQueue.cc"

int main(int argc, char* argv[]) {

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

  std::unique_ptr<OutputerBase> out;
  if(argc == 7) {
    std::string name(argv[6]);
    auto pos = name.find('.');
    if(name.substr(pos) == ".pds") {
      out = std::make_unique<PDSOutputer>(name);
    } else {
      std::cout <<"unknown output file extension "<<name.substr(pos)<<std::endl;
      return 1;
    }
  } else {
    out = std::make_unique<Outputer>();
  }

  std::function<std::unique_ptr<SourceBase>(std::string const&, unsigned long long)> factory;
  std::string fileName(argv[1]);
  {
    auto pos = fileName.find('.');
    if( fileName.substr(pos) == ".root") {
      factory = [](std::string const& iName, unsigned long long iNEvents) {
        return std::make_unique<RootSource>(iName, iNEvents);
      };
    }
    else if( fileName.substr(pos) == ".pds") {
      factory = [](std::string const& iName, unsigned long long iNEvents) {
        return std::make_unique<PDSSource>(iName, iNEvents);
      };
    }
    else {
      std::cout <<"unknown file type "<<fileName.substr(pos)<<std::endl;
      return 1;
    }

  }

  lanes.reserve(nLanes);
  for(unsigned int i = 0; i< nLanes; ++i) {
    lanes.emplace_back(i, factory(argv[1], nEvents), scale);
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
  std::cout <<"Read file "<<argv[1]<<"\n"
	    <<"# threads "<<parallelism<<"\n"
	    <<"# concurrent events "<<nLanes <<"\n"
	    <<"time scale "<<scale<<"\n";
  std::cout <<"Event processing time: "<<eventTime.count()<<"us"<<std::endl;
  std::cout <<"number events: "<<ievt.load() -nLanes<<std::endl;
  std::cout <<"----------"<<std::endl;

  std::chrono::microseconds sourceTime = std::chrono::microseconds::zero();
  std::chrono::microseconds serializerTime = std::chrono::microseconds::zero();

  std::vector<std::pair<std::string_view, std::chrono::microseconds>> serializerTimes;
  serializerTimes.reserve(lanes[0].serializers().size());
  bool isFirst = true;
  for(auto const& lane: lanes) {
    sourceTime += lane.sourceAccumulatedTime();
    if(isFirst) {
      isFirst = false;
      for(auto& s: lane.serializers()) {
	serializerTimes.emplace_back(s.name(), s.accumulatedTime());
	serializerTime += s.accumulatedTime();
      }
    } else {
      int i =0;
      for(auto& s: lane.serializers()) {
	serializerTimes[i++].second += s.accumulatedTime();
	serializerTime += s.accumulatedTime();
      }
    }
  }

  std::cout <<"\nSource time: "<<sourceTime.count()<<"us\n"<<std::endl;

  std::sort(serializerTimes.begin(),serializerTimes.end(), [](auto const& iLHS, auto const& iRHS) {
      return iLHS.second > iRHS.second;
    });

  std::cout <<"Serialization total time: "<<serializerTime.count()<<"us\n";
  std::cout <<"Serialization times\n";
  for(auto const& p: serializerTimes) {
    std::cout <<"time: "<<p.second.count()<<"us "<<std::setprecision(4)<<(100.*p.second.count()/serializerTime.count())<<"%\tname: "<<p.first<<"\n";
  }
}
