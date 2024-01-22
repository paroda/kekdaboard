#include "string.h"
#include "flash_store.h"

/*
 * 64K Block per dataset
 * Page-0 (256 bytes)
 *   byte-0 : id
 *   bytes-1 to 255 : position tracking bits (8*255=2040 bits)
 *     bit=0 : version present
 *     bit=1 : no data
 *     at start all tracking bit would be 1
 *     with write of 1st version the 1st bit is turned 0
 *     with each write of a new version, the next bit is turned 0
 *     when all bits exhausted, start again by turning all 1
 * Page-1 to 255 : dataset versions (256*255=65280 bytes)
 *   2040 versions
 *   each version size : 32 bytes
 *   total size : 32*2040=65280 bytes
 *   2040 versions tracked by 2040 bits above
 */

#define BASE_ADDR 65536 // reserved 1st 64K block
#define BASE_POS 8 // 1st 8 positions (8x32=256, 1 page) are reserved for position tracking
#define MAX_POS 2047 // total 2040 versions possible
#define BLOCK_SIZE 65536 // use one 64K block per dataset
#define SECTOR_SIZE 4096 // 4K sector - minimum which can be erased at a time
#define PAGE_SIZE 256

void (*store_read)(uint32_t addr, uint8_t* buf, size_t len);
void (*store_program)(uint32_t addr, const uint8_t* buf, size_t len);
void (*store_erase)(uint32_t addr);

void flash_create_store(uint8_t count, uint8_t* ids, flash_dataset_t** fds,
                        void (*read)(uint32_t addr, uint8_t* buf, size_t len),
                        void (*page_program)(uint32_t addr, const uint8_t* buf, size_t len),
                        void (*sector_erase)(uint32_t addr)) {
    for(unsigned int i=0; i<count; i++) {
        flash_dataset_t* fd = (flash_dataset_t*) malloc(sizeof(flash_dataset_t));
        fd->id = ids[i];
        fd->addr = BASE_ADDR + BLOCK_SIZE * i;
        fd->pos = BASE_POS;
        fd->need_flash_init = true;
        memset(fd->data, 0, FLASH_DATASET_SIZE);
        fds[i] = fd;
    }
    store_read = read;
    store_program = page_program;
    store_erase = sector_erase;
}

void flash_free_dataset(flash_dataset_t* fd) {
    free(fd);
}

void flash_store_load(flash_dataset_t* fd) {
    uint8_t buf[PAGE_SIZE];
    store_read(fd->addr, buf, PAGE_SIZE);
    if(buf[0]!=fd->id) return; // flash data invalid, continue with init data
    // check the position tracker, all bits upto and including current position should be 0
    // and the all the rest must be 1
    int i;
    uint8_t mask, bit_pos;
    uint16_t byte_pos, pos;
    for(i=BASE_POS/8; i<PAGE_SIZE; i++) if(buf[i]>0) break;
    byte_pos = i;
    for(i=0, mask=0x80; mask>0; i++, mask>>=1) if(buf[byte_pos] & mask) break;
    if(i==0) {
        byte_pos--;
        bit_pos=7;
    } else {
        bit_pos=i-1;
    }
    pos = byte_pos * 8 + bit_pos;
    if(pos<BASE_POS) return;
    if(buf[byte_pos]!=(0xFF>>(bit_pos+1))) return;
    for(i=byte_pos+1; i<PAGE_SIZE; i++) if(buf[i]!=0xFF) return;
    // read the data
    fd->pos = pos;
    fd->need_flash_init = false;
    store_read(fd->addr+(pos*FLASH_DATASET_SIZE), fd->data, FLASH_DATASET_SIZE);
}

void flash_store_save(flash_dataset_t* fd) {
    uint16_t pos = fd->pos;
    if(!fd->need_flash_init) {
        if(pos<MAX_POS) pos++;
        else pos=BASE_POS;
    } else {
        fd->need_flash_init = false;
    }
    uint32_t addr = fd->addr + (pos * FLASH_DATASET_SIZE);
    if(pos==BASE_POS) {
        store_erase(fd->addr);
        uint8_t buf[2] = {fd->id, 0x7F};
        store_program(fd->addr, buf, 2);
    } else {
        if(addr%SECTOR_SIZE == 0){
            store_erase(addr);
        }
        uint8_t track = 0xFF>>((pos%8)+1);
        store_program(fd->addr+(pos/8), &track, 1);
    }
    store_program(addr, fd->data, FLASH_DATASET_SIZE);
    fd->pos = pos;
}
