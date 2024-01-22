#include <stdio.h>
#include <string.h>

#include "../flash_store.h"

uint8_t store[655360];

void read(uint32_t addr, uint8_t* buf, size_t len) {
    memcpy(buf, store+addr, len);
}

void program(uint32_t addr, const uint8_t* buf, size_t len) {
    for(uint i=0; i<len; i++) {
        store[addr+i] &= buf[i];
    }
}

void erase(uint32_t addr) {
    memset(store+addr, 0xFF, 4096);
}

void print_fd(char* message, flash_dataset_t* fd) {
    printf("\n%s", message);
    printf("\n\nDataset %u, %d, %u, %u", fd->id, fd->need_flash_init, fd->pos, fd->addr);
    for(uint i=0; i<FLASH_DATASET_SIZE; i++) {
        if(i%16==0) printf("\n  data: ");
        printf("%02x", fd->data[i]);
        if(i%4==3) printf("  ");
    }
    printf("\n");
}

void printbuf(uint8_t* buf, size_t len) {
    for(uint i=0; i<len; i++) {
        if(i%16 == 15)
            printf("%02x\n", buf[i]);
        else if(i%4 == 3)
            printf("%02x, ", buf[i]);
        else
            printf("%02x ", buf[i]);
    }
}

void test1(flash_dataset_t* fd) {
    print_fd("initial", fd);

    flash_store_load(fd);
    print_fd("loaded", fd);

    fd->data[0] = 0xa0;
    fd->data[31] = 0xb0;
    flash_store_save(fd);
    print_fd("saved", fd);

    printbuf(store+fd->addr, 256);
    printf("\n");
    printbuf(store+fd->addr+256, 256);

    fd->data[0] = 0xc0;
    fd->data[16] = 0xd0;

    flash_store_load(fd);
    print_fd("modified then loaded", fd);

    fd->data[0] = 0xe0;
    flash_store_save(fd);
    print_fd("modified then saved", fd);

    printbuf(store+fd->addr, 256);
    printf("\n");
    printbuf(store+fd->addr+256, 256);

    fd->pos=8;
    fd->need_flash_init=true;
    memset(fd->data, 0, FLASH_DATASET_SIZE);
    print_fd("reset", fd);

    flash_store_load(fd);
    print_fd("loaded", fd);

    fd->pos=127;
    printf("\nNext sector before save\n");
    printbuf(store+fd->addr+4096, 256);
    flash_store_save(fd);
    print_fd("saved", fd);
    printbuf(store+fd->addr, 256);
    printbuf(store+fd->addr+4096, 256);

    fd->pos=2047;
    print_fd("exhausted", fd);
    flash_store_save(fd);
    print_fd("saved", fd);
    puts("");
    printbuf(store+fd->addr, 256);
    puts("");
    printbuf(store+fd->addr+256, 256);

}

int main(void) {
    printf("\nTesting flash store.");

    memset(store, 0, 655360);

    flash_dataset_t* fds[2];
    uint8_t ids[2] = {1, 2};
    flash_create_store(2, ids, fds, read, program, erase);

    test1(fds[0]);

    printf("\nEnd of test.");
}
