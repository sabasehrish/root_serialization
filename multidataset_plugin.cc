#include "multidataset_plugin.h"


static H5D_rw_multi_t *multi_datasets;
static hid_t *dataset_recycle;
static hid_t *memspace_recycle;
static hid_t *dataspace_recycle;

static int dataset_size;
static int dataset_size_limit;
static int dataset_recycle_size;
static int dataset_recycle_size_limit;
static int dataspace_recycle_size;
static int dataspace_recycle_size_limit;
static int memspace_recycle_size;
static int memspace_recycle_size_limit;
hsize_t total_data_size;

#define MEM_SIZE 2048

int init_multidataset() {
    dataset_size = 0;
    dataset_size_limit = 0;
    dataspace_recycle_size = 0;
    dataspace_recycle_size_limit = 0;
    memspace_recycle_size = 0;
    memspace_recycle_size_limit = 0;
    dataset_recycle_size_limit = 0;
    dataset_recycle_size = 0;
    return 0;
}

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

int register_multidataset(void *buf, hid_t did, hid_t dsid, hid_t msid, hid_t mtype, int write) {
    int dims[H5S_MAX_RANK];
    int mdims[H5S_MAX_RANK];
    size_t esize = H5Tget_size (mtype);
    H5Sget_simple_extent_dims (msid, dims, mdims);
    esize *= dims[0];

    if (dataset_size == dataset_size_limit) {
        if ( dataset_size_limit ) {
            dataset_size_limit *= 2;
            H5D_rw_multi_t *temp = (H5D_rw_multi_t*) malloc(dataset_size_limit*sizeof(H5D_rw_multi_t));
            memcpy(temp, multi_datasets, sizeof(H5D_rw_multi_t) * dataset_size);
            free(multi_datasets);
            multi_datasets = temp;
        } else {
            dataset_size_limit = MEM_SIZE;
            multi_datasets = (H5D_rw_multi_t*) malloc(dataset_size_limit*sizeof(H5D_rw_multi_t));
        }
    }

    multi_datasets[dataset_size].mem_space_id = msid;
    multi_datasets[dataset_size].dset_id = did;
    multi_datasets[dataset_size].dset_space_id = dsid;
    multi_datasets[dataset_size].mem_type_id = mtype;
    if (write) {
        multi_datasets[dataset_size].u.wbuf = (void*) malloc(esize);
        memcpy(multi_datasets[dataset_size].u.wbuf, buf, esize);
    } else {
        multi_datasets[dataset_size].u.rbuf = buf;
    }
    dataset_size++;
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

int flush_multidatasets() {
    int i;
    size_t esize;
    hsize_t dims[H5S_MAX_RANK], mdims[H5S_MAX_RANK];


    //printf("Rank %d number of datasets to be written %d\n", rank, dataset_size);
#if ENABLE_MULTIDATASET==1
    H5Dwrite_multi(H5P_DEFAULT, dataset_size, multi_datasets);
#else

    //printf("rank %d has dataset_size %lld\n", rank, (long long int) dataset_size);
    for ( i = 0; i < dataset_size; ++i ) {
        //MPI_Barrier(MPI_COMM_WORLD);
        H5Dwrite (multi_datasets[i].dset_id, multi_datasets[i].mem_type_id, multi_datasets[i].mem_space_id, multi_datasets[i].dset_space_id, H5P_DEFAULT, multi_datasets[i].u.wbuf);
    }
#endif

    for ( i = 0; i < dataset_size; ++i ) {
        H5Sget_simple_extent_dims (multi_datasets[i].mem_space_id, dims, mdims);
        esize = H5Tget_size (multi_datasets[i].mem_type_id);
        total_data_size += dims[0] * esize;
    }

    //printf("rank %d number of hyperslab called %d\n", rank, hyperslab_count);
    for ( i = 0; i < dataset_size; ++i ) {
        free(multi_datasets[i].u.wbuf);
    }
    if (dataset_size) {
        free(multi_datasets);
    }
    dataset_size = 0;
    dataset_size_limit = 0;
    return 0;
}
