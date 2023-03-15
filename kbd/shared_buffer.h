#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

void clear_shared_buffer(shared_buffer_t* sb);

/*
 * Allocate memory
 */
shared_buffer_t* new_shared_buffer(size_t size);

void free_shared_buffer(shared_buffer_t* sb);

void read_shared_buffer(shared_buffer_t* sb, uint64_t* ts, void* dst);

void write_shared_buffer(shared_buffer_t* sb, const uint64_t ts, const void* src);

#endif
