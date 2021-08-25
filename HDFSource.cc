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
  status = H5Oget_info_by_name (loc_id, name, &infobuf, H5O_INFO_BASIC, H5P_DEFAULT);
//We may want to see if H5O_INFO_ALL will be better
  switch (infobuf.type) {
    case H5O_TYPE_DATASET:
      if (strstr(name,"_sz") == NULL && strcmp(name, "Event_IDs") != 0){     
        productInfo_.push_back({name, dpid_});
        ++dpid_;
      }
      break;
    default:
      printf ( "  Other type: %s\n", name);
  }

  return 0;
}

HDFSource::HDFSource(std::string const& iName):
file_(File::open(iName.c_str())),
lumi_(Group::open(file_, "/Lumi"))
{
  dpid_ = 0;
  H5Literate (lumi_, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, op_func, NULL);
  
  dataProducts_.reserve(productInfo_.size());
  dataBuffers_.resize(productInfo_.size(), nullptr);
  classnames_=readClassNames();
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
  productInfo_.clear();
}


std::vector<std::string>
HDFSource::readClassNames() {
  std::vector<std::string> classnames;
  for (auto const& pi : productInfo_) {
    auto dset = Dataset::open(lumi_, (pi.name()).c_str());
    auto aid = Attribute::open(dset, "classname");
    auto tid = H5Aget_type(aid); 
    auto sz = H5Aget_storage_size(aid);
    char* attribute_name= new char[sz+1];
    H5Aread(aid, tid, &attribute_name);
    std::string s(attribute_name);
    H5free_memory(attribute_name);
    classnames.push_back(s);
  }
  return classnames;
}


std::pair<long unsigned int, long unsigned int>
HDFSource::getEventOffsets(long iEventIndex, std::string const& pname) {
  hsize_t nelements = 2;
  long unsigned int offsets[nelements];
  hsize_t coords[nelements]; 
  coords[0] = (iEventIndex == 0) ? iEventIndex: iEventIndex-1;
  coords[1] = iEventIndex;
  auto dset = Dataset::open(lumi_, (pname+"_sz").c_str());
  auto fspace = Dataspace::get_space(dset);
  auto mspace = Dataspace::create_simple(1, &nelements, NULL); 
  H5Sselect_elements(fspace, H5S_SELECT_SET, 2, (const hsize_t *)&coords);
  H5Dread(dset, H5T_STD_U64LE, mspace, fspace, H5P_DEFAULT, offsets); 
  if (iEventIndex == 0) {
    return {0, offsets[0]};
  } else {
   return {offsets[0], offsets[1]};
  }
}


bool
HDFSource::readEvent(long iEventIndex) {
  hsize_t ndims = 1;
  unsigned int run_num[ndims];
  unsigned int lumi_num[ndims];
  auto attr_r = Attribute::open(lumi_, "run");
  H5Aread(attr_r, H5T_STD_I32LE,run_num);
  auto attr_l = Attribute::open(lumi_, "lumisec");
  H5Aread(attr_l, H5T_STD_I32LE,lumi_num);
  auto dset = Dataset::open(lumi_, "Event_IDs");
  unsigned int eventID[ndims];
  hsize_t coords[ndims]; 
  coords[0] = iEventIndex;
  auto fspace = Dataspace::get_space(dset);
  auto mspace = Dataspace::create_simple(1, &ndims, NULL); 
  H5Sselect_elements(fspace, H5S_SELECT_SET, 1, (const hsize_t *)&coords);
  H5Dread(dset, H5T_STD_I32LE, mspace, fspace, H5P_DEFAULT, eventID); 
  eventID_ = {run_num[0], lumi_num[0], eventID[0]};
  hsize_t start[1];
  hsize_t count[1];
  long unsigned int begin;
  long unsigned int end;
  std::vector<product_t> products;
  products.reserve(productInfo_.size());
  char *product;
  for (auto pi : productInfo_) {
      std::tie(begin, end) = getEventOffsets(iEventIndex, pi.name());
      auto prods_dset = Dataset::open(lumi_, (pi.name()).c_str());
      auto prods_fspace = Dataspace::get_space(prods_dset);
      hsize_t dp_sz[1];
      dp_sz[0] = end-begin;
      auto prods_mspace = Dataspace::create_simple(1, &dp_sz[0], NULL); 
      //std::cout << "Reading: " << pi.name() << ", Begin at: " << begin << ", and end at : " << end << '\n';
      product = new char[dp_sz[0]];
      start[0]  = begin;
      count[0]  = dp_sz[0];
      prods_fspace.select_hyperslab(start, count);
      H5Dread(prods_dset, H5T_NATIVE_CHAR, prods_mspace, prods_fspace, H5P_DEFAULT, product);
      products.push_back({product, product + dp_sz[0]});
      delete[] product;
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
    bufferFile.Reset();
    ++productIndex;
    ++it;
  }
  assert(it==itEnd);
}
