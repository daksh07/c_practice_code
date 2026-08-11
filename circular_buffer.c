//#include <cstdint>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE 8   // small on purpose, so wraparound is easy to test



typedef struct {
    int16_t data[BUFFER_SIZE];
    volatile size_t write_idx;
    volatile size_t read_idx;
} circular_buffer_t;

void cb_init(circular_buffer_t *cb) {
    cb->write_idx = 0;
    cb->read_idx = 0;
}

// Simulate disabling/enabling interrupts for a critical section
static volatile bool interrupts_disabled = false;
void disable_interrupts(void) { interrupts_disabled = true; }
void enable_interrupts(void)  { interrupts_disabled = false; }

// Called from ISR context: push a new audio sample into the buffer
// Returns false if the buffer is full
bool cb_push(circular_buffer_t *cb, int16_t sample) {
    // TODO: 
    // 1. Compute what the next write_idx would be (with wraparound)
    // 2. Check if that equals read_idx (buffer full) -> return false
    // 3. Otherwise store the sample and update write_idx
    //    (keep any critical section as short as possible)
    size_t next_write_idx = (cb->write_idx + 1) % BUFFER_SIZE;
    if(next_write_idx != cb->read_idx){
        cb->data[cb->write_idx] = sample;
        disable_interrupts();
        cb->write_idx = next_write_idx % BUFFER_SIZE;
        enable_interrupts();
        return true;
    }
    else{
        return false;
    }
}

// Called from main loop: pop the oldest sample out of the buffer
// Returns false if the buffer is empty
bool cb_pop(circular_buffer_t *cb, int16_t *out_sample) {
    // TODO:
    // 1. Check if read_idx == write_idx (buffer empty) -> return false
    // 2. Otherwise read the sample and update read_idx (with wraparound)
    if(cb->read_idx != cb->write_idx){
        *out_sample = cb->data[cb->read_idx];
        cb->read_idx = (cb->read_idx + 1) % BUFFER_SIZE;
        return true;
    }
    else {
        return false;
    }
}

int main(void) {
    circular_buffer_t cb;
    cb_init(&cb);
    int16_t out_sample[BUFFER_SIZE];
    int16_t sample[8] = {10, 9, 4, 3, 22, 12, 14, 1};
    // TODO: test harness
    // 1. Fill the buffer until it reports full (should be BUFFER_SIZE - 1 items, not BUFFER_SIZE)
    // 2. Try pushing one more -> should fail
    // 3. Pop a couple of samples, then push again -> should succeed (wraparound works)
    // 4. Drain the buffer fully, confirm order of samples matches insertion order
    for(uint8_t i = 0; i<=(sizeof(sample)/sizeof(sample[0]))-1; i++){
        //printf("i value %d\n", i);
        if(cb_push(&cb, sample[i]) == false){
            printf("Buffer full! Byte dropped!\n");
        }
        //printf("Pushed %d\n", cb.data[i]);
    }
    if(cb_push(&cb, 7)==false){
        printf("Buffer full\n");
    }
    cb_pop(&cb, &out_sample[0]);
    printf("Pop [0]: %d\n", out_sample[0]);
    cb_pop(&cb, &out_sample[1]);
    printf("Pop [1]: %d\n", out_sample[1]);
    cb_push(&cb, 13);
    cb_push(&cb, 8);
    for(uint8_t j = 0; j <= BUFFER_SIZE - 1; j++){
        if(cb_pop(&cb, &out_sample[j])){
            printf("Pop: %d\n", out_sample[j]);
        }
        else {
            printf("Buffer empty!\n");
        }
    }
    return 0;
}