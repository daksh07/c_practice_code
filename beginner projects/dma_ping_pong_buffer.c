#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE 4

typedef struct {
    int16_t buffer_a[BUFFER_SIZE];
    int16_t buffer_b[BUFFER_SIZE];
    volatile bool dma_filling_a;
    volatile bool buffer_ready;
} ping_pong_t;

void pp_init(ping_pong_t *pp) {
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        pp->buffer_a[i] = 0;
        pp->buffer_b[i] = 0;
    }
    pp->dma_filling_a = true;
    pp->buffer_ready = false;
}

void dma_isr_simulate(ping_pong_t *pp, int16_t *new_samples, size_t n) {
    if(pp->dma_filling_a == true){
        for (size_t i = 0; i < n; i++) {
            pp->buffer_a[i] = new_samples[i];
    }
        pp->dma_filling_a = false;
        pp->buffer_ready = true;
        printf("buffer a filled! \n");
    }else{
        for (size_t i = 0; i < n; i++) {
            pp->buffer_b[i] = new_samples[i];
    }
        pp->dma_filling_a = true;
        pp->buffer_ready = true;
        printf("buffer b filled! \n");
    }
}

bool process_ready_buffer(ping_pong_t *pp, int16_t *out, size_t n) {
    if (pp->buffer_ready == true){
        if(pp->dma_filling_a == false){
            for(size_t i = 0; i < n; i++){
                out[i] = pp->buffer_a[i];
                printf("copying %d to buffer a\n",out[i]);
            }
            printf("buffer a processing complete!\n");
            pp->buffer_ready = false;
            return true;
        }else {
            for(size_t i = 0; i < n; i++){
                out[i] = pp->buffer_b[i];
                printf("copying %d to buffer b\n",out[i]);
            }
            printf("buffer b processing complete\n");
            pp->buffer_ready = false;
            return true;
        }
    }else {
        return false;
    }
}

int main(void) {
    ping_pong_t pp_data;
    int16_t out_data[BUFFER_SIZE] = {0};
    pp_init(&pp_data);
    int16_t test_buffer_a[4] = {40, 23, 60, 87};
    int16_t test_buffer_b[4] = {50, 33, 90, 27};
    dma_isr_simulate(&pp_data, test_buffer_a, 4);
    process_ready_buffer(&pp_data, out_data, BUFFER_SIZE);
    dma_isr_simulate(&pp_data, test_buffer_b, 4);
    process_ready_buffer(&pp_data, out_data, BUFFER_SIZE);
    if(!(process_ready_buffer(&pp_data, out_data, BUFFER_SIZE))){printf("Buffer not ready!\n");}
    return 0;
}