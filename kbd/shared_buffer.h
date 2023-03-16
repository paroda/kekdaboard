#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <hardware/sync.h>

/*
 * Shared buffer for concurrent operation - one writer, one reader.
 * Use timestamp to track change.
 */

typedef struct {
    size_t size;
    uint8_t* buff;
    volatile uint64_t ts;

    spin_lock_t* spin_lock;
} shared_buffer_t;

void clear_shared_buffer(shared_buffer_t* sb);

/*
 * Allocate memory
 */
shared_buffer_t* new_shared_buffer(size_t size, spin_lock_t* spin_lock);

void free_shared_buffer(shared_buffer_t* sb);

void read_shared_buffer(shared_buffer_t* sb, uint64_t* ts, void* dst);

void write_shared_buffer(shared_buffer_t* sb, const uint64_t ts, const void* src);

#endif
