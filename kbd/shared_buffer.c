#include <string.h>

#include "shared_buffer.h"

void clear_shared_buffer(shared_buffer_t* sb) {
    memset(sb->buff, 0, sb->size);
    sb->ts_start = 0;
    sb->ts_end = 0;
}

shared_buffer_t* new_shared_buffer(size_t size) {
    shared_buffer_t* sb = (shared_buffer_t*) malloc(sizeof(shared_buffer_t));
    sb->size = size;
    sb->buff = (uint8_t*) malloc(sizeof(uint8_t) * size);
    sb->ts_start = sb->ts_end = 0;
    return sb;
}

void free_shared_buffer(shared_buffer_t* sb) {
    free(sb->buff);
    free(sb);
}

void read_shared_buffer(shared_buffer_t* sb, uint64_t* ts, uint8_t* dst) {
    uint64_t ts_start = 0, ts_end = 0;
    do {
        ts_start = sb->ts_start;
        memcpy(dst, sb->buff, sb->size);
        ts_end = sb->ts_end;
    } while(ts_start != ts_end);
    *ts = ts_start;
}

void write_shared_buffer(shared_buffer_t* sb, const uint64_t ts, const uint8_t* src) {
    sb->ts_start = ts;
    memcpy(sb->buff, src, sb->size);
    sb->ts_end = ts;
}
