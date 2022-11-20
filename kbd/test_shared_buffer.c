#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

#include "shared_buffer.h"

#define DATA_SIZE 30

static shared_buffer_t* sb;

void set_array_data(uint8_t* dst, uint8_t value) {
    for(int i=0; i<DATA_SIZE; i++) dst[i]=value;
}

void* thread_write(void* vargp) {
    static uint8_t d = 1;

    /* sleep(1); */
    for(int j=0; j<10000; j++) {
        uint8_t data_in[DATA_SIZE];
        set_array_data(data_in, d++);
        write_shared_buffer(sb, 1, data_in);
    }
    return NULL;
}

void test(void) {
    sb = new_shared_buffer(DATA_SIZE);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, thread_write, NULL);

    /* sleep(1); */
    for(int j=0; j<100; j++) {
        uint8_t data_out[DATA_SIZE];
        read_shared_buffer(sb, data_out);
        printf("\ndata_out=[ ");
        for(int i=0; i<DATA_SIZE; i++) printf("%d, ", data_out[i]);
        printf("]\n");
    }

    pthread_join(thread_id, NULL);
}

int main(void) {
    printf("\nTest shared_buffer\n");

    test();

    printf("\nEnd of Test shared_buffer\n");
}
