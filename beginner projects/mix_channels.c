#include <stdio.h>
#include <stdint.h>
#include <limits.h>

// Mixes four signed 16-bit audio samples into one, without overflow.
int16_t mix_channels(int16_t ch1, int16_t ch2, int16_t ch3, int16_t ch4) {
    // TODO:
    // 1. Cast each channel up to a wider type (int32_t) BEFORE summing
    // 2. Sum all four in that wide accumulator
    // 3. Clamp the sum to INT16_MIN/INT16_MAX (same pattern as audio_gain)
    // 4. Cast back down to int16_t and return
    int32_t xch1 = ch1;
    int32_t xch2 = ch2;
    int32_t xch3 = ch3;
    int32_t xch4 = ch4;

    int32_t sum = xch1 + xch2 + xch3 + xch4;
    printf("sum_calc :%d\n",sum);
    if (sum > INT16_MAX){
        return INT16_MAX;
    }
    if (sum < INT16_MIN){
        return INT16_MIN;
    }
    return (int16_t)sum;
}

int main(void) {
    // TODO: test harness — think about what combinations actually
    // exercise the overflow path vs. the normal path:
    // 1. Four small/quiet samples that sum well within range -> no clamping needed
    // 2. Four samples that are individually valid but sum past INT16_MAX -> should clamp high
    // 3. Four very negative samples that sum past INT16_MIN -> should clamp low
    // 4. A mix of positive and negative that cancels out -> should return near 0
    int16_t samples[4][4] = {{20, 30, 40, 50}, {20000, 10000, 12312, 23124}, {-23984, -23812, -24938, -31947}, {-2, -1, 1, 2}};
    for(uint8_t i = 0; i < 4; i++){
        printf("Sum :%d\n", mix_channels(samples[i][0], samples[i][1], samples[i][2], samples[i][3]));
    }
    return 0;
}