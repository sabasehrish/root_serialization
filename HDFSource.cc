#include "HDFSource.h"

#include "TClass.h"
#include "TBufferFile.h"

using namespace cce::tf;

HDFSource::HDFSource(std::string const& iName):
file_(iName, File::ReadOnly),
lumi_(file_.getGroup("/Lumi/")), 
productInfo_(readProductInfo()), 
classnames_(readClassNames())
{
  dataProducts_.reserve(productInfo_.size());
  dataBuffers_.resize(productInfo_.size(), nullptr);
  size_t index = 0;
  for (auto p: productInfo_) {
    TClass* cls = TClass::GetClass(classnames_[p.classIndex()].c_str());
    //std::cout << p.name() << ", " << classnames_[p.classIndex()].c_str() << std::endl;
    dataBuffers_[index] = cls->New();
    assert(cls);
    dataProducts_.emplace_back(index, &dataBuffers_[index], p.name(), cls, &delayedRetriever_);
    ++index;
  }
}


HDFSource::~HDFSource() {
  auto it = dataProducts_.begin();
  for( void * b: dataBuffers_) {
    it->classType()->Destructor(b);
    ++it;
  }
}

//
std::vector<HDFSource::ProductInfo>
HDFSource::readProductInfo() {
  std::vector<HDFSource::ProductInfo> productinfo;
  uint32_t i = 0;
  for (auto p : lumi_.listObjectNames()) {
    //Event_IDs dataset is a special one
    if (p.compare(p.size()-3,3,"_sz") != 0 && p != "Event_IDs") {
      productinfo.push_back({p,i});
      ++i;
    }
  }
  return productinfo; 
}

std::vector<std::string>
HDFSource::readClassNames() {
  std::vector<std::string> classnames;
  for (auto pi : productInfo_) {
    auto d = lumi_.getDataSet(pi.name());
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
    d.select({static_cast<unsigned long int>(iEventIndex-1)}, {2}).read(st_end); 
  return {st_end[0], st_end[1]};
}

bool
HDFSource::readEvent(long iEventIndex) {
  int run_num;
  int lumi_num;
  lumi_.getAttribute("run").read(run_num);
  lumi_.getAttribute("lumisec").read(lumi_num);
  std::vector<char> product;
  std::vector<product_t> products;
  products.reserve(productInfo_.size());
  long unsigned int begin; 
  long unsigned int end;
  int i = 0;
  auto evtds = lumi_.getDataSet("Event_IDs");
  int eventID;
  evtds.select({iEventIndex}, {1}).read(eventID);
  eventID_ = {run_num, lumi_num, eventID}; 
  for (auto pi : productInfo_) { 
      std::tie(begin, end) = getEventOffsets(iEventIndex, pi.name());
      auto dset = lumi_.getDataSet(pi.name());
      auto ds = dset.getSpace();
      dset.select({begin}, {end-begin}).read(product); 
    //  std::cout << pi.name() << ", " << classnames_[i] << "Begin at: " << begin << ", and size of dp: " << (end-begin) << " , " << product.size() << "\n";
      ++i; 
      products.push_back(product);
  }
  deserializeDataProducts(products.begin(), products.end());    
  return true;
}

void HDFSource::deserializeDataProducts(buffer_iterator it, buffer_iterator itEnd) { 
  int productIndex = 0;
  TBufferFile bufferFile{TBuffer::kRead};
  while(it < itEnd) {
    auto const & product = *(it);
    auto storedSize = product.size();
    bufferFile.SetBuffer(const_cast<char*>(reinterpret_cast<char const*>(&product[0])), storedSize, kFALSE); 
    dataProducts_[productIndex].classType()->ReadBuffer(bufferFile, dataBuffers_[productIndex]);
    dataProducts_[productIndex].setSize(bufferFile.Length());
    //std::cout <<"Product Name: " << dataProducts_[productIndex].name()  << "Product Index "<< productIndex << ", " << dataProducts_[productIndex].classType() << ", storedSize " << storedSize  <<" Buffer size "<<bufferFile.Length() << ", and Buffer data: " << dataBuffers_[productIndex]<< std::endl;
    //if (bufferFile.Length() != storedSize) return;
    bufferFile.Reset();
    ++productIndex; 
    ++it;
  }
  assert(it==itEnd);
}
