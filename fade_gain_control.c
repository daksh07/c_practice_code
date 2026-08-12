#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

void fade_out(int16_t *buffer, size_t num_samples, size_t fade_duration_samples) {
    // TODO:
    // 1. Compute the gain step per sample (float, from 1.0 down to 0.0
    //    over fade_duration_samples)
    // 2. Loop through the buffer. For each sample within the fade duration,
    //    multiply by the current gain, then decrease gain by the step
    // 3. For samples beyond fade_duration_samples, what should happen?
    //    (Think back to test case 2 above)
    float gain = 1;
    float new_gain = 0;
    if(fade_duration_samples > 1){
        gain = 1/((float)fade_duration_samples - 1);
        new_gain = 1;
    }
    //printf("Gain step %f\n",gain); 
    for(size_t i = 0; i < num_samples; i++){
        if(i < fade_duration_samples){
            //printf("Scaled gain %f\n",new_gain);
            printf("Sample before fade %d\n", buffer[i]);
            float scaled_sample = (float)buffer[i];
            scaled_sample = scaled_sample * new_gain;
            buffer[i] = (int16_t)scaled_sample;
            new_gain = new_gain - gain;
            printf("After fade: %d\n", buffer[i]);
        }
        if(i >= fade_duration_samples){
            buffer[i] = 0;
            printf("After fade: %d\n", buffer[i]);
        }
    }
}

int main(void) {
    // TODO: build a test buffer, call fade_out, print before/after values
    int16_t test_buffer[8] = {245, 432,123, 343, 545, 523, 168, 837};
    fade_out(test_buffer, 8, 7);
    // to confirm the ramp behaves as expected
    return 0;
}