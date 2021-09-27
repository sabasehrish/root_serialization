#ifndef MULTIDATASET_PLUGIN_H
#define MULTIDATASET_PLUGIN_H

//#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <hdf5.h>
#define ENABLE_MULTIDATASET 0
#define MULTIDATASET_DEFINE 0

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

int init_multidataset();
int register_dataset_recycle(hid_t did);
int register_dataspace_recycle(hid_t dsid);
int register_memspace_recycle(hid_t msid);
int register_multidataset(void *buf, hid_t did, hid_t dsid, hid_t msid, hid_t mtype, int write);
int dataset_recycle_all();
int dataspace_recycle_all();
int memspace_recycle_all();
int flush_multidatasets();
#endif
