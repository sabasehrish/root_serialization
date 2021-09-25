#ifndef H5_TIMING_H

#define H5_TIMING_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#define TIMING_DEBUG 1

typedef struct{
    double start;
    double end;
    char *name;
} H5Timer;

typedef struct{
    H5Timer *timer_array;
    size_t max_size;
    size_t size;
} H5TimerArray;

int init_timers();
int register_dataset_timer_start(const char *name);
int register_dataset_timer_end();
int register_dataset_sz_timer_start(const char *name);
int register_dataset_sz_timer_end();
int register_dataset_read_timer_start(const char *name);
int register_dataset_read_timer_end();
int register_dataset_sz_read_timer_start(const char *name);
int register_dataset_sz_read_timer_end();
int finalize_timers();

#endif
