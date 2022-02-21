#ifndef MULTIDATASET_PLUGIN_H
#define MULTIDATASET_PLUGIN_H

//#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <hdf5.h>
#include "H5Timing.h"
#define ENABLE_MULTIDATASET 0
#define MULTIDATASET_DEFINE 1

static int max_batch_size_g = 2;
static int hdf_method_g = -1;
static int total_n_events_g = -1;


#if MULTIDATASET_DEFINE == 1
typedef struct H5D_rw_multi_t
{
    hid_t dset_id;          /* dataset ID */
    hid_t dset_space_id;    /* dataset selection dataspace ID */
    hid_t mem_type_id;      /* memory datatype ID */
    hid_t mem_space_id;     /* memory selection dataspace ID */
    union {
        void *rbuf;         /* pointer to read buffer */
        const void *wbuf;   /* pointer to write buffer */
    } u;
} H5D_rw_multi_t;
#endif

typedef struct multidataset_array {
    char *name;
    hsize_t *start;
    hsize_t *end;
    hsize_t last_end;
    hid_t did;
    hid_t mtype;      /* memory datatype ID */
    char **temp_mem;
    int request_size;
    int request_size_limit;
} multidataset_array;

int set_max_batch_size(int max_batch_size);
int get_max_batch_size();
int set_hdf5_method(int hdf5_method);
int get_hdf5_method();
int set_total_n_events(int total_n_events);
int get_total_n_events();

int init_multidataset();
int finalize_multidataset();
//hid_t get_dataset_id(const char* name, hid_t gid);
/*
int register_dataset_recycle(hid_t did);
int register_dataspace_recycle(hid_t dsid);
int register_memspace_recycle(hid_t msid);
int register_multidataset(const char* name, void *buf, hid_t did, hid_t dsid, hid_t msid, hid_t mtype, int write);
int dataset_recycle_all();
int dataspace_recycle_all();
int memspace_recycle_all();
*/
int register_multidataset_request_append(const char *name, hid_t gid, void *buf, hsize_t data_size, hid_t mtype);
int flush_multidatasets();
//int check_write_status();
#endif
