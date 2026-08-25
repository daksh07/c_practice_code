#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE 8

typedef struct {
    int16_t data[BUFFER_SIZE];
    volatile size_t write_idx;
    volatile size_t read_idx;
} circular_buffer_t;

void cb_init(circular_buffer_t *cb) {
    cb->read_idx = 0;
    cb->write_idx = 0;
    for(int16_t i = 0; i < BUFFER_SIZE; i++){
        cb->data[i] = 0;
    }
}

bool cb_push(circular_buffer_t *cb, int16_t sample) {
    static int16_t next_head = 0;
    next_head = (cb->write_idx + 1) % BUFFER_SIZE;
    if(next_head != cb->read_idx){
        cb->data[cb->write_idx] = sample;
        printf("Data pushed: %d\n", cb->data[cb->write_idx]);
        cb->write_idx = (cb->write_idx + 1)% BUFFER_SIZE;
        return true;
    }
    return false;
}

bool cb_pop(circular_buffer_t *cb, int16_t *out_sample) {
    if(cb->read_idx != cb->write_idx){
        for (int16_t i = 0; i < (3); i++) {
            out_sample[i] = cb->data[i];
            cb->read_idx = (cb->read_idx + 1) % BUFFER_SIZE;
            printf("Data popped: %d\n", out_sample[i]);
        }
        return true;
    }
    return false;
}

int main(void) {
    circular_buffer_t buffer;
    cb_init(&buffer);
    int16_t sample_data[3] = {0, 1, 3};
    int16_t out_buff[3] = {0};
    for (int16_t i = 0; i < (sizeof(sample_data)/sizeof(sample_data[0])); i++) {
        cb_push(&buffer, sample_data[i]);
    }
    for (int16_t i = 0; i < (sizeof(sample_data)/sizeof(sample_data[0])); i++) {
        cb_pop(&buffer, out_buff);
    }
    return 0;
}