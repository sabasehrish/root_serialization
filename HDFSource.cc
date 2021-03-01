#include "HDFSource.h"

using namespace cce::tf;

HDFSource::HDFSource(std::string const& iName):
file_(iName, File::ReadOnly)
//events_(file_.getDataSet("/Lumi/EventIDs")), 
{}


bool
HDFSource::readEvent(long iEventIndex) {
  Group lumi = file_.getGroup("/Lumi/");
  int run_num;
  int lumi_num;
  lumi.getAttribute("run").read(run_num);
  lumi.getAttribute("lumisec").read(lumi_num);
  std::cout << "Number Objects in this group" << lumi.getNumberObjects();
  std::vector<int> st_end{2};
  std::vector<char> products;
  for (auto i : lumi.listObjectNames()) { 
    // for the ones with _sz, use iEventIndex and lastIndex to get the size
    // for each data product in this event, print the start and end index to read
    // use that index to actually select and read dataset. 
    // We may want to read a batch and then assemble instead of this event by event approach
    if (i.compare(i.size()-3,3,"_sz") == 0) {
      auto d = lumi.getDataSet(i);
      if (iEventIndex == 0) {
        d.read(st_end);
        //st_end.insert(st_end.begin(), 0);
      }
      //else  
        //d.select({iEventIndex-1}, {2}).read(st_end); 
      
      std::cout << "\n" << i << ", " << d.getDimensions()[0] << ", " << st_end[0] << ", " << st_end[1];
    }
    //i.select({st_end[0]}, {st_end[1]}).read(products);
  }
  return true;
}
