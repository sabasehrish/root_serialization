#include "HDFSource.h"

#include "TClass.h"
#include "TBufferFile.h"

using namespace cce::tf;

HDFSource::HDFSource(std::string const& iName):
file_(iName, File::ReadOnly),
lumi_(file_.getGroup("/Lumi/")), 
//events_(lumi_.getDataSet("EventIDs")),
productnames_(readProductNames()), 
classnames_(readClassNames())
{
  dataBuffers_.reserve(numberOfDataProducts());
  size_t index = 0;
  for (auto p: productnames_) {
    TClass* cls = TClass::GetClass(classnames_[index].c_str());
    //dataBuffers_[index] = cls->New();
    assert(cls);
    dataProducts_.emplace_back(index, reinterpret_cast<void**>(&dataBuffers_[index]), p, cls, &delayedRetriever_);
    ++index;
  }
}

//
std::vector<std::string>
HDFSource::readProductNames() {
  std::vector<std::string> productnames;
  for (auto i : lumi_.listObjectNames()) {
    //Event_IDs dataset is a special one
    if (i.compare(i.size()-3,3,"_sz") != 0 && i != "Event_IDs") {
      productnames.push_back(i);
    }
  }
  return productnames; 
}

std::vector<std::string>
HDFSource::readClassNames() {
  std::vector<std::string> classnames;
  for (auto i : productnames_) {
    auto d = lumi_.getDataSet(i);
    std::vector<std::string> all_attribute_keys = d.listAttributeNames();
  //  std::cout << i << ", " << all_attribute_keys.size() << "\n";
    auto a = d.getAttribute(all_attribute_keys[0]);
    std::string classname;
    a.read(classname);
    classnames.push_back(classname);
  } 
  return classnames;
}

std::pair<long unsigned int, long unsigned int> 
HDFSource::getEventOffsets(long iEventIndex, std::string pname) {
  std::vector<long unsigned int> st_end;
  auto d = lumi_.getDataSet(pname+"_sz");
  if (iEventIndex == 0) {
    d.read(st_end);
    st_end.insert(st_end.begin(), 0);
  }
  else  
    d.select({static_cast<int>(iEventIndex)-1}, {2}).read(st_end); 
  return {st_end[0], st_end[1]};
}

bool
HDFSource::readEvent(long iEventIndex) {
  int run_num;
  int lumi_num;
  lumi_.getAttribute("run").read(run_num);
  lumi_.getAttribute("lumisec").read(lumi_num);
  //std::vector<long unsigned int> st_end;
  std::vector<char> product;
  std::vector<product_t> products;
  products.reserve(productnames_.size());
  long unsigned int begin; 
  long unsigned int end;
  int i = 0; 
  for (auto pname : productnames_) { 
      std::tie(begin, end) = getEventOffsets(iEventIndex, pname);
      auto dset = lumi_.getDataSet(pname);
      auto ds = dset.getSpace();
      dset.select({begin}, {end-begin}).read(product); 
      std::cout << pname << ", " << classnames_[i] << ": Size of dp: " << (end-begin) << " , " << product.size() << "\n";
      ++i; 
      products.push_back(product);
     //auto dr = DataProductRetriever(i, &products, pname, TClass::GetClass(classnames_[i].c_str()), delayedRetriever_);
 //     dataProducts_.emplace_back(i, reinterpret_cast<void**>(&dataBuffers_[i]), pname, TClass::GetClass(classnames_[i].c_str()), &delayedRetriever_);
      //make DataRetreiver here
      //TBufferFile readBufferFile{TBuffer::kRead};
      //readBufferFile.SetBuffer(&products.front(), products.size(), kFALSE);
      //TClass* cls = TClass::GetClass(classnames_[i].c_str());
      //cls->ReadBuffer(readBufferFile, dataBuffers_);
      //readBufferFile.Reset();
      //++i;
  }
  deserializeDataProducts(products.begin(), products.end());    
  return true;
}

void HDFSource::deserializeDataProducts(buffer_iterator it, buffer_iterator itEnd) { 
  TBufferFile bufferFile{TBuffer::kRead};
  int productIndex = 0;
  while(it < itEnd) {
    auto product = *(it++);
    auto storedSize = product.size();
   
    //std::cout <<"storedSize "<<storedSize<<" "<<storedSize*4<<std::endl;
    bufferFile.SetBuffer(const_cast<char*>(reinterpret_cast<char const*>(&*it)), storedSize, kFALSE);
    dataProducts_[productIndex].classType()->ReadBuffer(bufferFile, reinterpret_cast<void**>(&dataBuffers_[productIndex]));
    dataProducts_[productIndex].setSize(bufferFile.Length());
  //std::cout <<" size "<<bufferFile.Length()<<"\n";
    bufferFile.Reset();
    ++productIndex; 
  }
  assert(it==itEnd);
}
