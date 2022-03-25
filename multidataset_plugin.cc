#include "multidataset_plugin.h"


static std::map<std::string, multidataset_array*> multi_datasets;

int init_multidataset() {
    char *p = getenv("HEP_IO_TYPE");
    if ( p != NULL ) {
        set_hdf5_method(atoi(p));
    } else {
        set_hdf5_method(1);
    }
    return 0;
}

int finalize_multidataset() {
    int i, j;
    std::map<std::string, multidataset_array*>::iterator it;
    std::vector<char*>::iterator it2;
    for ( it = multi_datasets.begin(); it != multi_datasets.end(); ++it ) {
        for ( it2 = it->second->temp_mem->begin(); it2 != it->second->temp_mem->end(); ++it2 ) {
            free(*it2);
        }
        if (it->second->did != -1) {
            H5Dclose(it->second->did);
        }
	delete it->second->start;
	delete it->second->end;
	delete it->second->temp_mem;
        free(it->second);
    }
    return 0;
}

int set_hdf5_method(int hdf5_method) {
    hdf5_method_g = hdf5_method;
    return 0;
}

int get_hdf5_method() {
    return hdf5_method_g;
}

static hid_t get_dataset_id(const char* name, hid_t gid) {
    std::string s(name);
    if ( multi_datasets.find(s) == multi_datasets.end()) {
        multi_datasets[s]->did = H5Dopen2(gid, name, H5P_DEFAULT);
    }
    return multi_datasets[s]->did;
}

static int wrap_hdf5_spaces(int total_requests, hsize_t *start, hsize_t *end, hid_t did, hid_t* dsid_ptr, hid_t *msid_ptr) {
    const hsize_t ndims = 1;
    hsize_t old_dims[ndims]; //our datasets are 1D
    hsize_t new_dims[ndims];
    hsize_t max_dims[ndims]; //= {H5S_UNLIMITED};
    hsize_t max_offset, data_size, total_data_size;
    hid_t dsid, msid;
    int i;

    dsid = H5Dget_space(did);
    H5Sget_simple_extent_dims(dsid, old_dims, max_dims);
    
    max_offset = end[0];
    for ( i = 1; i < total_requests; ++i ) {
        if ( max_offset < end[i] ) {
            max_offset = end[i];
        }
    }
    if (max_offset > old_dims[0]) {
        new_dims[0] = max_offset;
        H5Dset_extent(did, new_dims);
        H5Sclose(dsid);
        dsid = H5Dget_space(did);
    }

    data_size = end[0] - start[0];
    H5Sselect_hyperslab(dsid, H5S_SELECT_SET, start, NULL, &data_size, NULL);
    total_data_size = data_size;
    for ( i = 1; i < total_requests; ++i ) {
        data_size = end[i] - start[i];
        total_data_size += data_size;
        H5Sselect_hyperslab(dsid, H5S_SELECT_OR, start + i, NULL, &data_size, NULL);
    }
    max_dims[0] = H5S_UNLIMITED;
    msid = H5Screate_simple(ndims, &total_data_size, max_dims);

    *dsid_ptr = dsid;
    *msid_ptr = msid;
    return 0;
}

int register_multidataset_request(const char *name, hid_t gid, void *buf, hsize_t start, hsize_t end, hid_t mtype) {
    std::string s(name);
    char *temp_mem;
    size_t esize = H5Tget_size (mtype) * (end - start);
    std::map<std::string, multidataset_array*>::iterator it;

    it = multi_datasets.find(s);
    if ( it == multi_datasets.end()) {
        multidataset_array *multi_dataset = (multidataset_array *) malloc(sizeof(multidataset_array));
	multi_dataset->start = new std::vector<hsize_t>;
	multi_dataset->end =new std::vector<hsize_t>;
	multi_dataset->temp_mem = new std::vector<char*>;
        multi_datasets[s] = multi_dataset;
	it = multi_datasets.find(s);
	it->second->did = -1;
    }
    if (it->second->did == -1 ) {
        it->second->did = H5Dopen2(gid, name, H5P_DEFAULT);
    }
    it->second->start->push_back(start);
    it->second->end->push_back(end);
    temp_mem = (char*) malloc(esize);
    it->second->temp_mem->push_back(temp_mem);
    memcpy(temp_mem, buf, esize);
    it->second->last_end = end;
    it->second->mtype = mtype;

    return 0;
}

int register_multidataset_request_append(const char *name, hid_t gid, void *buf, hsize_t data_size, hid_t mtype) {
    std::string s(name);
    std::map<std::string, multidataset_array*>::iterator it;
    hsize_t start, end;
    it = multi_datasets.find(s);
    if ( it != multi_datasets.end() ){
        start = it->second->last_end;
        end = it->second->last_end + data_size;
    } else {
        start = 0;
        end = data_size;
    }
    register_multidataset_request(name, gid, buf, start, end, mtype);
    return 0;
}

static int merge_requests(std::vector<hsize_t> *start, std::vector<hsize_t> *end, int request_size, std::vector<char*> *buf, hsize_t **new_start, hsize_t **new_end, char** new_buf, hid_t mtype, int *request_size_ptr) {
    int i, index;
    int merged_requests = request_size;
    char* ptr;
    size_t esize = H5Tget_size (mtype);
    size_t total_data_size = end[0][0] - start[0][0];

    for ( i = 1; i < request_size; ++i ) {
        total_data_size += end[0][i] - start[0][i];
        if ( end[0][i-1] == start[0][i] ) {
            merged_requests--;
        }
    }

    *new_start = (hsize_t*) malloc(sizeof(hsize_t) * merged_requests * 2);
    *new_end = new_start[0] + merged_requests;

    index = 0;
    new_start[0][0] = start[0][0];
    new_end[0][0] = end[0][0];

    *new_buf = (char*) malloc(esize * total_data_size);
    ptr = *new_buf;
    memcpy(ptr, buf[0][0], (end[0][0] - start[0][0]) * esize);
    ptr += (end[0][0] - start[0][0]) * esize;
    free(buf[0][0]);
    for ( i = 1; i < request_size; ++i ) {
        memcpy(ptr, buf[0][i], (end[0][i] - start[0][i]) * esize);
        ptr += (end[0][i] - start[0][i]) * esize;
        free(buf[0][i]);

        if ( end[0][i-1] < start[0][i] ) {
            index++;
            new_start[0][index] = start[0][i];
        }
        new_end[0][index] = end[0][i];
    }
    *request_size_ptr = merged_requests;
    return 0;
}

int flush_multidatasets() {
    int i;
    size_t esize;
    hsize_t dims[H5S_MAX_RANK], mdims[H5S_MAX_RANK];
    H5D_rw_multi_t *multi_datasets_temp;
    hsize_t *new_start, *new_end;
    int new_request_size;
    hid_t msid, dsid;
    int dataset_size = multi_datasets.size();
    std::map<std::string, multidataset_array*>::iterator it;
    char **temp_buf = (char**) malloc(sizeof(char*) * dataset_size);
#ifdef H5_TIMING_ENABLE
    double start_time;
#endif
    i = 0;
    //printf("Rank %d number of datasets to be written %d\n", rank, dataset_size);
#if ENABLE_MULTIDATASET==1
    #ifdef H5_TIMING_ENABLE
    increment_H5Dwrite();
    #endif
    multi_datasets_temp = (H5D_rw_multi_t*) malloc(sizeof(H5D_rw_multi_t) * dataset_size);
    for ( it = multi_datasets.begin(); it != multi_datasets.end(); ++it ) {
        if (it->second->did == -1) {
	    i++;
            continue;
        }

        //MPI_Barrier(MPI_COMM_WORLD);
        #ifdef H5_TIMING_ENABLE
        increment_H5Dwrite();
        #endif

        merge_requests(it->second->start, it->second->end, it->second->start->size(), it->second->temp_mem, &new_start, &new_end, &(temp_buf[i]), it->second->mtype, &new_request_size);
        multi_datasets_temp[i].dset_id = it->second->did;
        multi_datasets_temp[i].mem_type_id = it->second->mtype;
        multi_datasets_temp[i].u.wbuf = temp_buf[i];

        wrap_hdf5_spaces(new_request_size, new_start, new_end, it->second->did, &(multi_datasets_temp[i].dset_space_id), &(multi_datasets_temp[i].mem_space_id));
	i++;
    }

    H5Dwrite_multi(H5P_DEFAULT, dataset_size, multi_datasets_temp);

    for ( it = multi_datasets.begin(); it != multi_datasets.end(); ++it ) {
        if (it->second->did == -1) {
            continue;
        }
        H5Sclose(multi_datasets_temp[i].dset_space_id);
        H5Sclose(multi_datasets_temp[i].mem_space_id);
        H5Dclose(multi_datasets_temp[i].dset_id);
	delete it->second->start;
	delete it->second->end;
	delete it->second->temp_mem;
        free(temp_buf[i]);
    }

    free(multi_datasets_temp);
#else
    //printf("rank %d has dataset_size %lld\n", rank, (long long int) dataset_size);
    for ( it = multi_datasets.begin(); it != multi_datasets.end(); ++it ) {
        if (it->second->did == -1) {
	    i++;
            continue;
        }
        #ifdef H5_TIMING_ENABLE
        increment_H5Dwrite();
        #endif
#ifdef H5_TIMING_ENABLE
        register_timer_start(&start_time);
#endif
        merge_requests(it->second->start, it->second->end, it->second->start->size(), it->second->temp_mem, &new_start, &new_end, temp_buf + i, it->second->mtype, &new_request_size);

#ifdef H5_TIMING_ENABLE
        register_merge_requests_timer_end(start_time);
#endif
#ifdef H5_TIMING_ENABLE
        register_timer_start(&start_time);
#endif
        wrap_hdf5_spaces(new_request_size, new_start, new_end, it->second->did, &dsid, &msid);
#ifdef H5_TIMING_ENABLE
        register_wrap_requests_timer_end(start_time);
#endif

#ifdef H5_TIMING_ENABLE
        register_timer_start(&start_time);
#endif
        H5Dwrite (it->second->did, it->second->mtype, msid, dsid, H5P_DEFAULT, temp_buf[i]);
#ifdef H5_TIMING_ENABLE
        register_H5Dwrite_timer_end(start_time);
#endif

        H5Sclose(dsid);
        H5Sclose(msid);
#ifdef H5_TIMING_ENABLE
        register_timer_start(&start_time);
#endif
        H5Dclose(it->second->did);
#ifdef H5_TIMING_ENABLE
        register_H5Dclose_timer_end(start_time);
#endif
	delete it->second->start;
	delete it->second->end;
	delete it->second->temp_mem;
        free(temp_buf[i]);
	i++;
    }
#endif
    multi_datasets.clear();

    free(temp_buf);
    return 0;
}
