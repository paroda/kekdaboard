#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Shared buffer for concurrent operation - one writer, one reader.
     * Use timestamp to track change.
     */

    typedef struct {
        uint8_t* buff;
        size_t size;
        // timestamp - us since boot
        volatile uint64_t ts_start;
        volatile uint64_t ts_end;
    } shared_buffer_t;

    void clear_shared_buffer(shared_buffer_t* sb) {
        memset(sb->buff, 0, sb->size);
        sb->ts_start = 0;
        sb->ts_end = 0;
    }

    /*
     * Allocate memory
     */
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

#ifdef __cplusplus
}
#endif

#endif
