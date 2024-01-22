#include <string.h>

#include "shared_buffer.h"

void clear_shared_buffer(shared_buffer_t* sb) {
    uint32_t saved_irq = spin_lock_blocking(sb->spin_lock);
    memset(sb->buff, 0, sb->size);
    sb->ts = 0;
    spin_unlock(sb->spin_lock, saved_irq);
}

shared_buffer_t* new_shared_buffer(size_t size, spin_lock_t* spin_lock) {
    shared_buffer_t* sb = (shared_buffer_t*) malloc(sizeof(shared_buffer_t));
    sb->size = size;
    sb->buff = (uint8_t*) malloc(sizeof(uint8_t) * size);
    sb->spin_lock = spin_lock;
    clear_shared_buffer(sb);
    return sb;
}

void free_shared_buffer(shared_buffer_t* sb) {
    free(sb->buff);
    free(sb);
}

void read_shared_buffer(shared_buffer_t* sb, uint64_t* ts, void* dst) {
    uint32_t saved_irq = spin_lock_blocking(sb->spin_lock);
    memcpy(dst, sb->buff, sb->size);
    *ts = sb->ts;
    spin_unlock(sb->spin_lock, saved_irq);
}

void write_shared_buffer(shared_buffer_t* sb, const uint64_t ts, const void* src) {
    uint32_t saved_irq = spin_lock_blocking(sb->spin_lock);
    sb->ts = ts;
    memcpy(sb->buff, src, sb->size);
    spin_unlock(sb->spin_lock, saved_irq);
}
