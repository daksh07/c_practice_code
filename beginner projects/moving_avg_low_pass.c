#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

void apply_moving_average_filter(int16_t *input, int16_t *output, size_t num_samples, uint8_t window_size) {
    // TODO:
    // 1. For each output sample i, sum the current sample and up to
    //    (window_size - 1) samples before it
    // 2. Watch out for reading before index 0 when i is near the start
    // 3. Use a wide enough accumulator type for the sum before dividing
    //    (think back to the mixer exercise)
    // 4. Divide by however many samples actually went into that average
    //    (not always window_size, near the start of the buffer)
    float temp = 0.0;
    float sum = 0.0;
    size_t i = 0;
    uint8_t j = 0;
    for (i = 0; i < num_samples ; i++) {
        printf("Input: %d\n",input[i]);
        if (i >= window_size){
            sum = 0.0;
            uint8_t start_win = i - (window_size - 1);
            for (uint8_t j = start_win; j <= i; j++){
                sum = sum + (float)input[j];
            }
            temp = sum/window_size;
            output[i] = (int16_t)temp;
            printf("Output: %d\n",output[i]);
        }
        else{
            sum = 0.0;
            for (uint8_t j = 0; j <= i ; j++) {
                sum = sum + (float)input[j];
            }
            temp = sum/(i+1);
            output[i] = (int16_t)temp;
            printf("Output: %d\n",output[i]);
        }  
    }
}

int main(void) {
    // TODO: build a test buffer with a spike, run the filter, print
    int16_t in_buf[10] = {100, 100, 100, 100, 10000, 100, 100, 100, 100, 100};
    int16_t out_buf[10] = {0};
    apply_moving_average_filter(in_buf, out_buf, 10, 3);
    // input vs output side by side to see the smoothing effect
    return 0;
}