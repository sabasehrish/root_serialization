#if !defined(HDFCxx_h)
#define HDFCxx_h

#include "hdf5.h"

#include <vector>

namespace cce::tf {


inline hid_t string_type() {
auto atype = H5Tcopy(H5T_C_S1);
H5Tset_size(atype, H5T_VARIABLE);
H5Tset_cset(atype, H5T_CSET_UTF8);
H5Tset_strpad(atype,H5T_STR_NULLTERM);
return atype;
}
template<typename T> hid_t H5type_for = -1;
template<> inline hid_t H5type_for<int> = H5T_STD_I32LE;
template<> inline hid_t H5type_for<char> = H5T_STD_I8LE;
template<> inline hid_t H5type_for<size_t> = H5T_STD_U64LE;
template<> inline hid_t H5type_for<std::string> = string_type();

//wrapper for File
class File {
  public:
    static File create(const char *name) {
      return File(H5Fcreate(name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
    } 
    static File open(const char *name) {
      return File(H5Fopen(name, H5F_ACC_RDONLY, H5P_DEFAULT));
    }
    ~File() {
      H5Fclose(file_);
    }
    operator hid_t() const {return file_;}
  private:
    explicit File(hid_t fid):file_(fid) {}
    hid_t file_;
};
//wrapper for Group
class Group {
  public:
    static Group create(hid_t id, const char *name) {
      return Group(H5Gcreate2(id, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    } 
    static Group open(hid_t id, const char *name) {
      return Group(H5Gopen2(id, name, H5P_DEFAULT));
    }
    ~Group() {
      H5Gclose(group_);
    }
    operator hid_t() const {return group_;}
  private:
    explicit Group(hid_t gid):group_(gid) {}
    hid_t group_;
};

//wrapper for Dataset
class Dataset {
  public: 
    static Dataset create(hid_t id, const char *name, hid_t dtype_id, hid_t space_id, hid_t dcpl_id) {
    
     return Dataset(H5Dcreate2(id, name, dtype_id, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)); 
    } 
    static Dataset open(hid_t id, const char *name){
      return Dataset(H5Dopen2(id, name, H5P_DEFAULT));}
    ~Dataset() { 
       H5Dclose(dataset_);
    }
    operator hid_t() const {return dataset_;}
    
    template<typename T>
    auto 
    write(hid_t dspace, hid_t filespace, std::vector<T> const & data) {
      return H5Dwrite(dataset_, H5type_for<T>, dspace, filespace, H5P_DEFAULT, &data[0]);
    }
 
    template<typename T>
    auto 
    read(hid_t dspace, hid_t filespace, std::vector<T> &data) {
      return H5Dread(dataset_, H5type_for<T>, dspace, filespace, H5P_DEFAULT, &data[0]);
    }
  private: 
      explicit Dataset(hid_t ds_id):dataset_(ds_id) {}
      hid_t dataset_;
};

//wrapper for Attributes
class Attribute {
  public: 
    template<typename T>
    static Attribute create(hid_t id, const char *name, hid_t aid) {
     return Attribute(H5Acreate2(id, name, H5type_for<T>, aid, H5P_DEFAULT, H5P_DEFAULT)); 
    } 
    static Attribute open(hid_t id, const char *name){
      return Attribute(H5Aopen(id, name, H5P_DEFAULT));
    }
    ~Attribute() { 
       H5Aclose(attribute_);
    }
    operator hid_t() const {return attribute_;}
    template<typename T>
    auto write(T const & data){
        return H5Awrite(attribute_, H5type_for<T>, &data);
    } 
   private: 
      explicit Attribute(hid_t a_id):attribute_(a_id) {}
      hid_t attribute_;
};

//wrapper for Dataspace
//

class Dataspace {
  public:
    static Dataspace create_simple(hsize_t ndims, hsize_t *dims, hsize_t *maxdims) {
      return Dataspace(H5Screate_simple(ndims, dims, maxdims));
    }
    static Dataspace get_space(hid_t hid) {
      return Dataspace(H5Dget_space(hid));
    } 
    ~Dataspace() {
      H5Sclose(dspace_);
    }
    operator hid_t() const {return dspace_;}
  private:
    explicit Dataspace(hid_t s):dspace_(s) {}
    hid_t dspace_;
};
}
#endif
