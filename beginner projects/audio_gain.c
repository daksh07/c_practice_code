#include <stdio.h>
#include <stdint.h>
#include <limits.h>

int16_t audio_gain(int16_t sample, float gain) {
    int32_t output = sample * gain;
    if(output > INT16_MAX){
        output = INT16_MAX;
    }
    if (output < INT16_MIN) {
        output = INT16_MIN;
    }
    return (int16_t)output;
}

int main(void) {
    int16_t sample[2] = {20000, -20000};
    float gain[4] = {1.0, 1.89, 0.0, 0.5 };
    int16_t output_sample = 0;

    for(uint8_t i = 0; i < (sizeof(sample)/sizeof(sample[0])); i++){
        for(uint8_t j = 0; j< (sizeof(gain)/sizeof(gain[0])); j++){
            output_sample = audio_gain(sample[i], gain[j]);
            printf("Input sample: %d\n", sample[i]);
            //printf("Gain: %d\n", gain[j]);
            printf("Output sample: %d\n", output_sample);
        }
    }
    return 0;
}

/*Test cases it needs to handle correctly:

A normal in-range sample with gain = 1.0 → returns the sample unchanged
A sample near the top of the range with gain > 1.0 → should clamp to INT16_MAX (32767), not wrap around
A sample near the bottom of the range (very negative) with gain > 1.0 → should clamp to INT16_MIN (-32768), not wrap around
gain = 0.0 → should always return 0, regardless of the sample
A fractional gain like 0.5 → should scale down correctly (e.g. halve the sample)

*/