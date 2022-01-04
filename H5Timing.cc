#include "H5Timing.h"

#ifdef H5_TIMING_ENABLE

static H5TimerArray *dataset_timers;
static H5TimerArray *dataset_sz_timers;
static H5TimerArray *dataset_read_timers;
static H5TimerArray *dataset_sz_read_timers;
static int H5Dwrite_count;
static int H5Dread_count;
static double H5Dwrite_time;
static double H5Dread_time;
static double total_start_time;
static double total_end_time;

int init_timers() {
    struct timeval temp_time;
    dataset_timers = (H5TimerArray*) malloc(sizeof(H5TimerArray)*4);
    dataset_sz_timers = dataset_timers + 1;
    dataset_read_timers = dataset_timers + 2;
    dataset_sz_read_timers = dataset_timers + 3;

    dataset_timers->max_size = 64;
    dataset_timers->size = 0;
    dataset_timers->timer_array = (H5Timer*) malloc(sizeof(H5Timer) * dataset_timers->max_size);

    dataset_sz_timers->max_size = 64;
    dataset_sz_timers->size = 0;
    dataset_sz_timers->timer_array = (H5Timer*) malloc(sizeof(H5Timer) * dataset_sz_timers->max_size);

    dataset_read_timers->max_size = 64;
    dataset_read_timers->size = 0;
    dataset_read_timers->timer_array = (H5Timer*) malloc(sizeof(H5Timer) * dataset_timers->max_size);

    dataset_sz_read_timers->max_size = 64;
    dataset_sz_read_timers->size = 0;
    dataset_sz_read_timers->timer_array = (H5Timer*) malloc(sizeof(H5Timer) * dataset_sz_timers->max_size);

    gettimeofday(&temp_time, NULL);
    total_start_time = (temp_time.tv_usec + temp_time.tv_sec * 1000000) + .0;

    H5Dwrite_count = 0;
    H5Dread_count = 0;

    H5Dwrite_time = .0;
    H5Dread_time = .0;
    return 0;
}

static int check_timer_size(H5TimerArray *timers) {
    H5Timer *temp;
    if ( timers->size == timers->max_size ) {
        temp = (H5Timer*) malloc(sizeof(H5Timer) * timers->max_size * 2);
        memcpy(temp, timers->timer_array, sizeof(H5Timer) * timers->max_size);
        free(timers->timer_array);
        timers->timer_array = temp;
        timers->max_size *= 2;
        return 1;
    }
    return 0;
}

#define TIMER_START(timers) {                                   \
    struct timeval temp_time;                                       \
    check_timer_size(timers);                              \
    gettimeofday(&temp_time, NULL); \
    timers->timer_array[timers->size].start = (temp_time.tv_usec + temp_time.tv_sec * 1000000) + .0; \
    timers->timer_array[timers->size].name = (char*) malloc(sizeof(char) * (strlen(name) + 1)); \
    strcpy(timers->timer_array[timers->size].name, name); \
}

#define TIMER_END(timers, data_size) { \
    struct timeval temp_time;     \
    gettimeofday(&temp_time, NULL); \
    timers->timer_array[timers->size].end = (temp_time.tv_usec + temp_time.tv_sec * 1000000) + .0; \
    timers->timer_array[timers->size].data_size = data_size; \
    timers->size++; \
}

int register_H5Dwrite_timer_start(double *start_time) {
    struct timeval temp_time;
    gettimeofday(&temp_time, NULL);
    *start_time = (temp_time.tv_usec + temp_time.tv_sec * 1000000);
    return 0;
}

int register_H5Dwrite_timer_end(double start_time) {
    struct timeval temp_time;
    gettimeofday(&temp_time, NULL);
    H5Dwrite_time = (temp_time.tv_usec + temp_time.tv_sec * 1000000) - start_time;
    return 0;
}

int register_dataset_timer_start(const char *name) {
    TIMER_START(dataset_timers);
    return 0;
}
int register_dataset_timer_end(size_t data_size) {
    TIMER_END(dataset_timers, data_size);
    return 0;
}

int register_dataset_sz_timer_start(const char *name) {
    TIMER_START(dataset_sz_timers);
    return 0;
}

int register_dataset_sz_timer_end(size_t data_size) {
    TIMER_END(dataset_sz_timers, data_size);
    return 0;
}

int register_dataset_read_timer_start(const char *name) {
    TIMER_START(dataset_read_timers);
    return 0;
}
int register_dataset_read_timer_end(size_t data_size) {
    TIMER_END(dataset_read_timers, data_size);
    return 0;
}

int register_dataset_sz_read_timer_start(const char *name) {
    TIMER_START(dataset_sz_read_timers);
    return 0;
}

int register_dataset_sz_read_timer_end(size_t data_size) {
    TIMER_END(dataset_sz_read_timers, data_size);
    return 0;
}

int increment_H5Dwrite() {
    H5Dwrite_count++;
    return 0;
}

int increment_H5Dread() {
    H5Dread_count++;
    return 0;
}

int record_timer(H5TimerArray *timer, const char* filename) {
    FILE *stream;
    size_t i, total_mem_size;
    double total_time = 0, min_time = -1, max_time = -1;
    stream = fopen(filename, "w");
    fprintf(stream, "dataset_name,start,end,elapse,data_size\n");
    total_mem_size = 0;
    for ( i = 0; i < timer->size; ++i ) {
        fprintf(stream, "%s, %lf, %lf, %lf, %zu\n", timer->timer_array[i].name, timer->timer_array[i].start, timer->timer_array[i].end, (timer->timer_array[i].end - timer->timer_array[i].start)/1000000, timer->timer_array[i].data_size);
        total_time += (timer->timer_array[i].end - timer->timer_array[i].start)/1000000;
        if (min_time == -1 || min_time > timer->timer_array[i].start) {
            min_time = timer->timer_array[i].start;
        }
        if (max_time == -1 || max_time < timer->timer_array[i].end) {
            max_time = timer->timer_array[i].end;
        }
        total_mem_size += timer->timer_array[i].data_size;
    }
    fprintf(stream, "total,%lf,%lf,%lf,%zu\n", min_time, max_time, total_time, total_mem_size);
    fclose(stream);
    return 0;
}

int output_results() {
    printf("total H5Dwrite calls = %d, H5Dread calls = %d\n", H5Dwrite_count, H5Dread_count);
    if ( dataset_timers->size ) {
        record_timer(dataset_timers, "dataset_write_record.csv");
    }
    if ( dataset_sz_timers->size ) {
        record_timer(dataset_sz_timers, "dataset_sz_write_record.csv");
    }
    if ( dataset_read_timers->size ) {
        record_timer(dataset_read_timers, "dataset_read_record.csv");
    }
    if ( dataset_sz_read_timers->size ) {
        record_timer(dataset_sz_read_timers, "dataset_sz_read_record.csv");
    }
    return 0;
}

int finalize_timers() {
    size_t i;
    struct timeval temp_time;
    output_results();

    gettimeofday(&temp_time, NULL);
    total_end_time = (temp_time.tv_usec + temp_time.tv_sec * 1000000) + .0;
    printf("total program time is %lf, H5Dwrite time = %lf, H5Dread time = %lf\n", (total_end_time - total_start_time) / 1000000, H5Dwrite_time, H5Dread_time);

    for ( i = 0 ; i < dataset_timers->size; ++i ) {
        free(dataset_timers->timer_array[i].name);
    }

    for ( i = 0 ; i < dataset_sz_timers->size; ++i ) {
        free(dataset_sz_timers->timer_array[i].name);
    }

    for ( i = 0 ; i < dataset_read_timers->size; ++i ) {
        free(dataset_read_timers->timer_array[i].name);
    }

    for ( i = 0 ; i < dataset_sz_read_timers->size; ++i ) {
        free(dataset_sz_read_timers->timer_array[i].name);
    }

    free(dataset_timers->timer_array);
    free(dataset_sz_timers->timer_array);

    free(dataset_read_timers->timer_array);
    free(dataset_sz_read_timers->timer_array);

    free(dataset_timers);
    return 0;
}
#endif
