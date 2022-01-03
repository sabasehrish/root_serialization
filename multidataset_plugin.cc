#include "multidataset_plugin.h"


static multidataset_array *multi_datasets;
/*
static hid_t *dataset_recycle;
static hid_t *memspace_recycle;
static hid_t *dataspace_recycle;
*/
static int dataset_size;
static int dataset_size_limit;

/*
static int dataset_recycle_size;
static int dataset_recycle_size_limit;
static int dataspace_recycle_size;
static int dataspace_recycle_size_limit;
static int memspace_recycle_size;
static int memspace_recycle_size_limit;
*/
hsize_t total_data_size;

#define MEM_SIZE 2048
#define MAX_DATASET 65536

int init_multidataset() {
    dataset_size = 0;
    dataset_size_limit = 0;
/*
    dataspace_recycle_size = 0;
    dataspace_recycle_size_limit = 0;
    memspace_recycle_size = 0;
    memspace_recycle_size_limit = 0;
    dataset_recycle_size_limit = 0;
    dataset_recycle_size = 0;
*/
    return 0;
}

int finalize_multidataset() {
    int i, j;

    for ( i = 0; i < dataset_size; ++i ) {
        free(multi_datasets[i].name);
        free(multi_datasets[i].start);
        for ( j = 0; j < multi_datasets[i].request_size; ++j ) {
            free(multi_datasets[i].temp_mem[j]);
        }
        free(multi_datasets[i].temp_mem);
        if (multi_datasets[i].did != -1) {
            H5Dclose(multi_datasets[i].did);
        }
    }
    free(multi_datasets);
    dataset_size = 0;
    return 0;
}

#if 0
int register_dataset_recycle(hid_t did) {
    if (dataset_recycle_size == dataset_recycle_size_limit) {
        if ( dataset_recycle_size_limit > 0 ) {
            dataset_recycle_size_limit *= 2;
            hid_t *temp = (hid_t*) malloc(dataset_recycle_size_limit*sizeof(hid_t));
            memcpy(temp, dataset_recycle, sizeof(hid_t) * dataset_recycle_size);
            free(dataset_recycle);
            dataset_recycle = temp;
        } else {
            dataset_recycle_size_limit = MEM_SIZE;
            dataset_recycle = (hid_t*) malloc(dataset_recycle_size_limit*sizeof(hid_t));
        }
    }
    dataset_recycle[dataset_recycle_size] = did;
    dataset_recycle_size++;
    return 0;
}


int register_dataspace_recycle(hid_t dsid) {
    if (dataspace_recycle_size == dataspace_recycle_size_limit) {
        if ( dataspace_recycle_size_limit > 0 ) {
            dataspace_recycle_size_limit *= 2;
            hid_t *temp = (hid_t*) malloc(dataspace_recycle_size_limit*sizeof(hid_t));
            memcpy(temp, dataspace_recycle, sizeof(hid_t) * dataspace_recycle_size);
            free(dataspace_recycle);
            dataspace_recycle = temp;
        } else {
            dataspace_recycle_size_limit = MEM_SIZE;
            dataspace_recycle = (hid_t*) malloc(dataspace_recycle_size_limit*sizeof(hid_t));
        }
    }
    dataspace_recycle[dataspace_recycle_size] = dsid;
    dataspace_recycle_size++;
    return 0;
}

int register_memspace_recycle(hid_t msid) {
    if (memspace_recycle_size == memspace_recycle_size_limit) {
        if ( memspace_recycle_size_limit > 0 ) {
            memspace_recycle_size_limit *= 2;
            hid_t *temp = (hid_t*) malloc(memspace_recycle_size_limit*sizeof(hid_t));
            memcpy(temp, memspace_recycle, sizeof(hid_t) * memspace_recycle_size);
            free(memspace_recycle);
            memspace_recycle = temp;
        } else {
            memspace_recycle_size_limit = MEM_SIZE;
            memspace_recycle = (hid_t*) malloc(memspace_recycle_size_limit*sizeof(hid_t));
        }
    }
    memspace_recycle[memspace_recycle_size] = msid;
    memspace_recycle_size++;
    return 0;
}
#endif

static hid_t get_dataset_id(const char* name, hid_t gid) {
    int i;
    for ( i = 0; i < dataset_size; ++i ) {
        if (strcmp(name, multi_datasets[i].name) == 0) {
            if (multi_datasets[i].did != -1) {
                return multi_datasets[i].did;
            }
            break;
        }
    }
    multi_datasets[i].did = H5Dopen2(gid, name, H5P_DEFAULT);
    return multi_datasets[i].did;
}

static int wrap_hdf5_spaces(const char *name, int total_requests, hsize_t *start, hsize_t *end, hid_t did, hid_t* dsid_ptr, hid_t *msid_ptr) {
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
    size_t esize = H5Tget_size (mtype) * (end - start);
    hsize_t *temp_offset;
    int i;
    int index = -1;
    char** temp_mem;

    for ( i = 0; i < dataset_size; ++i ) {
        if ( strcmp(name, multi_datasets[i].name) == 0 ) {
            index = i;
            break;
        }
    }
    if ( index == -1 ) {
        if ( dataset_size == dataset_size_limit ) {
            if ( dataset_size_limit ) {
                dataset_size_limit *= 2;
                multidataset_array *temp = (multidataset_array*) malloc(dataset_size_limit*sizeof(multidataset_array));
                memcpy(temp, multi_datasets, sizeof(multidataset_array) * dataset_size);
                free(multi_datasets);
                multi_datasets = temp;

            } else {
                dataset_size_limit = MEM_SIZE;
                multi_datasets = (multidataset_array*) malloc(dataset_size_limit*sizeof(multidataset_array));
            }
        }
        index = dataset_size;
        multi_datasets[index].name = strdup(name);
        multi_datasets[index].did = H5Dopen2(gid, name, H5P_DEFAULT);
        multi_datasets[index].request_size_limit = MEM_SIZE;
        multi_datasets[index].request_size = 0;
        multi_datasets[index].temp_mem = (char**) malloc(sizeof(char**) * multi_datasets[index].request_size_limit);
        multi_datasets[index].start = (hsize_t*) malloc(2 * sizeof(hsize_t) * multi_datasets[index].request_size_limit);
        multi_datasets[index].end = multi_datasets[index].start + multi_datasets[index].request_size_limit;
        multi_datasets[index].mtype = mtype;

        dataset_size++;
    }

    if (multi_datasets[index].request_size_limit == multi_datasets[index].request_size) {
        multi_datasets[index].request_size_limit *= 2;
        temp_mem = (char**) malloc(sizeof(char*) * multi_datasets[index].request_size_limit);
        temp_offset = (hsize_t*) malloc(sizeof(hsize_t) * multi_datasets[index].request_size_limit * 2);
        memcpy(temp_mem, multi_datasets[index].temp_mem, sizeof(char*) * multi_datasets[index].request_size);
        memcpy(temp_offset, multi_datasets[index].start, sizeof(hsize_t) * multi_datasets[index].request_size);
        memcpy(temp_offset + multi_datasets[index].request_size_limit, multi_datasets[index].end, sizeof(hsize_t) * multi_datasets[index].request_size);

        free(multi_datasets[index].temp_mem);
        free(multi_datasets[index].start);

        multi_datasets[index].temp_mem = temp_mem;
        multi_datasets[index].start = temp_offset;
        multi_datasets[index].end = temp_offset + multi_datasets[index].request_size_limit;

    }
    if (multi_datasets[index].did == -1) {
        multi_datasets[index].did = H5Dopen2(gid, name, H5P_DEFAULT);
    }
    multi_datasets[index].start[multi_datasets[index].request_size] = start;
    multi_datasets[index].end[multi_datasets[index].request_size] = end;
    multi_datasets[index].temp_mem[multi_datasets[index].request_size] = (char*) malloc(esize);
    memcpy(multi_datasets[index].temp_mem[multi_datasets[index].request_size], buf, esize);
    multi_datasets[index].last_end = end;

    multi_datasets[index].request_size++;

    return 0;
}

int register_multidataset_request_append(const char *name, hid_t gid, void *buf, hsize_t data_size, hid_t mtype) {
    int i;
    int index = -1;
    hsize_t start, end;
    for ( i = 0; i < dataset_size; ++i ) {
        if ( strcmp(name, multi_datasets[i].name) == 0 ) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        start = multi_datasets[index].last_end;
        end = multi_datasets[index].last_end + data_size;
    } else {
        start = 0;
        end = data_size;
    }

    register_multidataset_request(name, gid, buf, start, end, mtype);
    return 0;
}

static int merge_requests(hsize_t *start, hsize_t *end, int request_size, char** buf, hsize_t **new_start, hsize_t **new_end, char** new_buf, hid_t mtype, int *request_size_ptr) {
    int i, index;
    int merged_requests = request_size;
    char* ptr;
    size_t esize = H5Tget_size (mtype);
    size_t total_data_size = end[0] - start[0];

    for ( i = 1; i < request_size; ++i ) {
        total_data_size += end[i] - start[i];
        if ( end[i-1] == start[i] ) {
            merged_requests--;
        }
    }
    *new_start = (hsize_t*) malloc(sizeof(hsize_t) * merged_requests * 2);
    *new_end = new_start[0] + merged_requests;

    index = 0;
    new_start[0][0] = start[0];
    new_end[0][0] = end[0];


    *new_buf = (char*) malloc(esize * total_data_size);
    ptr = *new_buf;
    memcpy(ptr, buf[0], (end[0] - start[0]) * esize);
    ptr += (end[0] - start[0]) * esize;
    free(buf[0]);
    for ( i = 1; i < request_size; ++i ) {
        memcpy(ptr, buf[i], (end[i] - start[i]) * esize);
        ptr += (end[i] - start[i]) * esize;
        free(buf[i]);

        if ( end[i-1] < start[i] ) {
            index++;
            new_start[0][index] = start[i];
        }
        new_end[0][index] = end[i];
    }
    *request_size_ptr = merged_requests;
    return 0;
}

#if 0
int register_multidataset(const char *name, void *buf, hid_t did, hid_t dsid, hid_t msid, hid_t mtype, int write) {
    int i;
    hsize_t dims[H5S_MAX_RANK];
    hsize_t mdims[H5S_MAX_RANK];
    hsize_t start[H5S_MAX_RANK];
    hsize_t end[H5S_MAX_RANK];
    char *tmp_buf;
    hsize_t data_size, total_data_size;
    hsize_t zero = 0;
    size_t esize = H5Tget_size (mtype);

    if (write) {
        for ( i = 0; i < dataset_size; ++i ) {
            if (strcmp(name, multi_datasets[i].name) == 0) {
                /* Extract data size from input memory space */
                H5Sget_simple_extent_dims(msid, &data_size, mdims);
                /* Reset dataspace for existing dataset */
                H5Sget_simple_extent_dims(multi_datasets[i].dsid, dims, mdims);
                dims[0] += data_size;
                H5Sget_select_bounds(multi_datasets[i].dsid, start, end );
                H5Sset_extent_simple( multi_datasets[i].dsid, 1, dims, dims );
                /* Reset the end size to slab size*/
                end[0] = end[0] + data_size;
                /* Add the new selection */
                H5Sselect_none(multi_datasets[i].dsid);
                H5Sselect_hyperslab(multi_datasets[i].dsid, H5S_SELECT_SET, start, NULL, end, NULL);
                H5Sclose(dsid);

                /* Reset the existing memory space directly to the current data size */
                H5Sget_simple_extent_dims(multi_datasets[i].msid, dims, mdims);
                dims[0] += data_size;
                total_data_size = dims[0];
                H5Sset_extent_simple( multi_datasets[i].msid, 1, dims, dims );
                H5Sselect_all( multi_datasets[i].msid );
                H5Sclose(msid);

                tmp_buf = multi_datasets[i].temp_mem;
                multi_datasets[i].temp_mem = (char*) malloc(esize * total_data_size);
                memcpy(temp_mem[dataset_size], tmp_buf, esize * (total_data_size - data_size) );
                memcpy(temp_mem[dataset_size] + esize * (total_data_size - data_size), buf, esize * data_size );

                free(tmp_buf);
                multi_datasets[dataset_size].u.wbuf = temp_mem[dataset_size];

                H5Dclose(did);
                return 0;
            }
        }
    }

    H5Sget_simple_extent_dims (msid, dims, mdims);
    esize *= dims[0];

    if (dataset_size == dataset_size_limit) {
        if ( dataset_size_limit ) {
            dataset_size_limit *= 2;
            multidataset_array *temp = (multidataset_array*) malloc(dataset_size_limit*sizeof(multidataset_array));
            memcpy(temp, multi_datasets, sizeof(multidataset_array) * dataset_size);
            free(multi_datasets);
            multi_datasets = temp;

            char **new_memory = (char**) malloc(dataset_size_limit*sizeof(char*));
            memcpy(new_memory, temp_mem, sizeof(char*) * dataset_size);
            free(temp_mem);
            temp_mem = new_memory;
        } else {
            dataset_size_limit = MEM_SIZE;
            temp_mem = (char**) malloc(sizeof(char*) * dataset_size_limit);
            multi_datasets = (multidataset_array*) malloc(dataset_size_limit*sizeof(multidataset_array));
        }
    }

    multi_datasets[dataset_size].msid = msid;
    multi_datasets[dataset_size].did = did;
    multi_datasets[dataset_size].dsid = dsid;
    multi_datasets[dataset_size].mtype = mtype;
    strcpy(multi_datasets[dataset_size].name, name);
    if (write) {
        temp_mem[dataset_size] = (char*) malloc(esize);
        memcpy(temp_mem[dataset_size], buf, esize);
        multi_datasets[dataset_size].u.wbuf = temp_mem[dataset_size];
    } else {
        multi_datasets[dataset_size].u.rbuf = buf;
    }
    dataset_size++;

    register_dataset_recycle(did);
    register_dataspace_recycle(dsid);
    register_memspace_recycle(msid);
    return 0;
}

int check_write_status() {
    if (dataset_size < MAX_DATASET) {
        return 0;
    }
#ifdef H5_TIMING_ENABLE
    register_dataset_timer_start("flush_all");
#endif
    flush_multidatasets();
    dataset_recycle_all();
    dataspace_recycle_all();
    memspace_recycle_all();
#ifdef H5_TIMING_ENABLE
    register_dataset_timer_end(total_data_size);
#endif

    return 0;
}

int dataset_recycle_all() {
    int i;
    for ( i = 0; i < dataset_recycle_size; ++i ) {
        if ( dataset_recycle[i] >= 0 ) {
            H5Dclose(dataset_recycle[i]);
        }
    }
    if (dataset_recycle_size) {
        free(dataset_recycle);
    }
    dataset_recycle_size = 0;
    dataset_recycle_size_limit = 0;
    return 0;
}


int dataspace_recycle_all() {
    int i;
    //printf("recycle %d dataspace\n", dataspace_recycle_size);
    for ( i = 0; i < dataspace_recycle_size; ++i ) {
        if ( dataspace_recycle[i] >= 0 ) {
            H5Sclose(dataspace_recycle[i]);
        }
    }
    if (dataspace_recycle_size) {
        free(dataspace_recycle);
    }
    dataspace_recycle_size = 0;
    dataspace_recycle_size_limit = 0;
    return 0;
}

int memspace_recycle_all() {
    int i;
    //printf("recycle %d memspace\n", memspace_recycle_size);
    for ( i = 0; i < memspace_recycle_size; ++i ) {
        if ( memspace_recycle[i] >= 0 ){
            H5Sclose(memspace_recycle[i]);
        }
    }
    if (memspace_recycle_size) {
        free(memspace_recycle);
    }
    memspace_recycle_size = 0;
    memspace_recycle_size_limit = 0;
    return 0;
}
#endif

int flush_multidatasets() {
    int i, j;
    size_t esize;
    hsize_t dims[H5S_MAX_RANK], mdims[H5S_MAX_RANK];
    H5D_rw_multi_t *multi_datasets_temp;
    hsize_t *new_start, *new_end;
    int new_request_size;
    hid_t msid, dsid;
    char **temp_buf = (char**) malloc(sizeof(char*) * dataset_size);

    //printf("Rank %d number of datasets to be written %d\n", rank, dataset_size);
#if ENABLE_MULTIDATASET==1
    #ifdef H5_TIMING_ENABLE
    increment_H5Dwrite();
    #endif
    multi_datasets_temp = (H5D_rw_multi_t*) malloc(sizeof(H5D_rw_multi_t) * dataset_size);

    for ( i = 0; i < dataset_size; ++i ) {
        if (multi_datasets[i].did == -1) {
            continue;
        }

        //MPI_Barrier(MPI_COMM_WORLD);
        #ifdef H5_TIMING_ENABLE
        increment_H5Dwrite();
        #endif

        merge_requests(multi_datasets[i].start, multi_datasets[i].end, multi_datasets[i].request_size, multi_datasets[i].buf, &new_start, &new_end, &(temp_buf[i]), multi_datasets[i].mtype, &new_request_size);
        multi_datasets_temp[i].dset_id = multi_datasets[i].did;
        multi_datasets_temp[i].mem_type_id = multi_datasets[i].mtype;
        multi_datasets_temp[i].u.wbuf = temp_buf[i];

        wrap_hdf5_spaces(multi_datasets[i].name, new_request_size, new_start, new_end, multi_datasets[i].did, &(multi_datasets_temp[i].dset_space_id), &(multi_datasets_temp[i].mem_space_id));
    }

    H5Dwrite_multi(H5P_DEFAULT, dataset_size, multi_datasets_temp);

    for ( i = 0; i < dataset_size; ++i ) {
        if (multi_datasets[i].did == -1) {
            continue;
        }
        H5Sclose(multi_datasets_temp[i].dset_space_id);
        H5Sclose(multi_datasets_temp[i].mem_space_id);
        H5Dclose(multi_datasets[i].did);
        multi_datasets[i].did = -1;
        free(temp_buf[i]);
    }

    free(multi_datasets_temp);
#else
    //printf("rank %d has dataset_size %lld\n", rank, (long long int) dataset_size);
    for ( i = 0; i < dataset_size; ++i ) {
        if (multi_datasets[i].did == -1) {
            continue;
        }
        #ifdef H5_TIMING_ENABLE
        increment_H5Dwrite();
        #endif
        merge_requests(multi_datasets[i].start, multi_datasets[i].end, multi_datasets[i].request_size, multi_datasets[i].temp_mem, &new_start, &new_end, &(temp_buf[i]), multi_datasets[i].mtype, &new_request_size);
        wrap_hdf5_spaces(multi_datasets[i].name, new_request_size, new_start, new_end, multi_datasets[i].did, &dsid, &msid);
        multi_datasets[i].request_size = 0;

        H5Dwrite (multi_datasets[i].did, multi_datasets[i].mtype, msid, dsid, H5P_DEFAULT, temp_buf[i]);

        H5Sclose(dsid);
        H5Sclose(msid);
        H5Dclose(multi_datasets[i].did);
        multi_datasets[i].did = -1;
        free(temp_buf[i]);
    }
#endif

    free(temp_buf);
    return 0;
}
