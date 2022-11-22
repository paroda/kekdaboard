#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __PICO_BUILD__
#include "pico/stdlib.h"
#else
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /***

     */

    /*
     * Shared buffer for concurrent operation - one writer, one reader.
     *
     * Writer can anytime write `ts_in_start`, `buff_in`, `ts_in_end` in that order.
     * The same timestamp must be written to both `ts_in_start` and `ts_in_end`.
     *
     * Reader can anytime read `buff_out` consistently. It reflects the value
     * of `buff_in` at the time of last run of `update_shared_buffer_to_read`,
     * which should be called in the reader thread only.
     */

    typedef struct {
        uint8_t* buff_in;
        uint8_t* buff_out;
        size_t size;
        // timestamp - us since boot
        uint64_t ts_in_start;
        uint64_t ts_in_end;
        uint64_t ts_out_start;
        uint64_t ts_out_end;
    } shared_buffer_t;

    void clear_shared_buffer(shared_buffer_t* sb) {
        memset(sb->buff_in, 0, sb->size);
        memset(sb->buff_out, 0, sb->size);
        sb->ts_in_start = 0;
        sb->ts_in_end = 0;
        sb->ts_out_start = 0;
        sb->ts_out_end = 0;
    }

    /*
     * Allocate memory
     */
    shared_buffer_t* new_shared_buffer(size_t size) {
        shared_buffer_t* sb = (shared_buffer_t*) malloc(sizeof (shared_buffer_t));
        sb->size = size;
        sb->buff_in = (uint8_t*) malloc(2*size);
        sb->buff_out = sb->buff_in + size;
        sb->ts_in_start = sb->ts_in_end = sb->ts_out_start = sb->ts_out_end = 0;
        return sb;
    }

    void free_shared_buffer(shared_buffer_t* sb) {
        free(sb->buff_in);
        free(sb->buff_out);
        free(sb);
    }

    /*
     * Must be called in reader thread.
     * returns true if there is new data, else false
     */
    bool update_shared_buffer_to_read(shared_buffer_t* sb) {
        if(sb->ts_out_start == sb->ts_in_start) {
            return false;
        }
        do {
            sb->ts_out_start = sb->ts_in_start;
            memcpy(sb->buff_out, sb->buff_in, sb->size);
            sb->ts_out_end = sb->ts_in_end;
        } while(sb->ts_out_start != sb->ts_out_end);
        return true;
    }

    void write_shared_buffer(shared_buffer_t* sb, uint64_t ts, uint8_t* src) {
        sb->ts_in_start = ts;
        memcpy(sb->buff_in, src, sb->size);
        sb->ts_in_end = ts;
    }

    /*
     * returns true if new data was read, else false
     */
    bool read_shared_buffer(shared_buffer_t* sb, uint8_t *dst) {
        if(update_shared_buffer_to_read(sb)) {
            memcpy(dst, sb->buff_out, sb->size);
            return true;
        }
        return false;
    }

#ifdef __cplusplus
}
#endif

#endif
