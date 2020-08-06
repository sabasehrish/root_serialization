#include "TROOT.h"
#include "TVirtualStreamerInfo.h"
#include "TObject.h"
#include <iostream>
#include <vector>
#include <string>
#include <atomic>

#include "Outputer.h"
#include "Lane.h"

#include "tbb/task_group.h"
#include "tbb/global_control.h"
#include "tbb/task_scheduler_init.h"


int main(int argc, char* argv[]) {

  if(not (argc > 1 and argc < 5) ) {
    std::cout <<"1 to 3 arguments required\n";
    return 1;
  }

  //Tell Root we want to be multi-threaded
  ROOT::EnableThreadSafety();
  
  //When threading, also have to keep ROOT from logging all TObjects into a list
  TObject::SetObjectStat(false);
  
  //Have to avoid having Streamers modify themselves after they have been used
  TVirtualStreamerInfo::Optimize(false);

  size_t parallelism = tbb::task_scheduler_init::default_num_threads();
  if(argc > 2) {
    parallelism = atoi(argv[2]);
  }
  tbb::global_control c(tbb::global_control::max_allowed_parallelism, parallelism);

  std::vector<Lane> lanes;
  unsigned int nLanes = 4;
  if(argc == 4) {
    nLanes = atoi(argv[3]);
  }

  lanes.reserve(nLanes);
  for(unsigned int i = 0; i< nLanes; ++i) {
    lanes.emplace_back(argv[1]);
  }
  Outputer out;
  std::atomic<long> ievt{0};
  tbb::task_group group;
  
  group.run([&]() {
      for(auto& lane : lanes) {
	lane.processEventsAsync(ievt, group, out);
      }
    });
    
  group.wait();
}
