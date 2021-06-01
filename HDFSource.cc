#include "HDFSource.h"

#include "TClass.h"
#include "TBufferFile.h"

using namespace cce::tf;

// C function (copied from HDF5 examples) that is passed as Operator function
// to H5Literate.  
// Prints the name and type of the object being examined.
herr_t
HDFSource::op_func (hid_t loc_id, const char *name, const H5L_info_t *info,
            void *operator_data)
{
  herr_t          status;
  H5O_info_t      infobuf;

 /*
 *      * Get type of the object and display its name and type.
 *        The name of the object is passed to this function by
 *        the Library.
 *  */
  status = H5Oget_info_by_name (loc_id, name, &infobuf, H5O_INFO_BASIC, H5P_DEFAULT);
//We may want to see if H5O_INFO_ALL will be better
  switch (infobuf.type) {
    case H5O_TYPE_GROUP:
      printf ("  Group: %s\n", name);
      break;
    case H5O_TYPE_DATASET:
      if (strstr(name,"_sz") == NULL && strcmp(name, "Event_IDs") != 0){     
        printf ("  Dataset: %s\n", name);
        printf ("  loc id: %d\n", dpid_);
        productInfo_.push_back({name, dpid_});
        ++dpid_;
      }
      break;
    case H5O_TYPE_NAMED_DATATYPE:
      printf ("  Datatype: %s\n", name);
      break;
    default:
      printf ( "  Unknown: %s\n", name);
  }

  return 0;
}

HDFSource::HDFSource(std::string const& iName):
file_(iName, File::ReadOnly),
lumi_(file_.getGroup("/Lumi/")),
file1_(H5Fopen(iName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT))
{
  dpid_ = 0;
  std::string gname = "/Lumi";
  lumi1_ = H5Gopen2(file1_, gname.c_str(), H5P_DEFAULT);
  H5Literate (lumi1_, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, op_func, NULL);
  
  dataProducts_.reserve(productInfo_.size());
  dataBuffers_.resize(productInfo_.size(), nullptr);
  classnames_=readClassNames();
  size_t index = 0;
  for (auto p: productInfo_) {
    TClass* cls = TClass::GetClass(classnames_[p.classIndex()].c_str());
    std::cout << p.name() << ", " << classnames_[p.classIndex()].c_str() << std::endl;
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
  productInfo_.clear();
  H5Gclose(lumi1_);
  H5Fclose(file1_);
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
  std::vector<long unsigned int> temp;
  auto d = lumi_.getDataSet(pname+"_sz");
  hid_t dset = H5Dopen2(lumi1_, (pname+"_sz").c_str(), H5P_DEFAULT);
  auto space = H5Dget_space (dset); 
  hsize_t stride[1];
  hsize_t start[1];
  if (iEventIndex == 0) {
    d.read(st_end);
 // C code 
    stride[0] = 1;
    start[0] = 0;
    auto status = H5Sselect_hyperslab (space, H5S_SELECT_SET, start, NULL, stride, NULL);
    status = H5Dread (dset, H5T_STD_I32LE, H5S_ALL, space, H5P_DEFAULT, &temp[0]);
    st_end.insert(st_end.begin(), 0);
  }
  else
  {
   // C code
   stride[0] = 2;
   start[0] = iEventIndex-1;
    auto status = H5Sselect_hyperslab (space, H5S_SELECT_SET, start, NULL, stride, NULL);
    //status = H5Dread (dset, H5T_STD_I32LE, space, H5S_ALL, H5P_DEFAULT, &temp[0]);
   // end
    d.select({static_cast<unsigned long int>(iEventIndex-1)}, {2}).read(st_end);
  }
  
  H5Dclose(dset);
  return {st_end[0], st_end[1]};
}

bool
HDFSource::readEvent(long iEventIndex) {
  unsigned int run_num;
  unsigned int lumi_num;
  lumi_.getAttribute("run").read(run_num);
  lumi_.getAttribute("lumisec").read(lumi_num);
  std::vector<char> product;
  std::vector<product_t> products;
  products.reserve(productInfo_.size());
  long unsigned int begin;
  long unsigned int end;
  int i = 0;
  auto evtds = lumi_.getDataSet("Event_IDs");
  unsigned int eventID;
  evtds.select({static_cast<unsigned long>(iEventIndex)}, {1}).read(eventID);
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
