#include <stdio.h>
#include <stdint.h>

uint32_t swap_endian_32(uint32_t value) {
    uint32_t byte1 = value & 0xff;
    uint32_t byte2 = (value & 0xff << 8) >> 8;
    uint32_t byte3 = (value & 0xff << 16) >> 16;
    uint32_t byte4 = (value & 0xff << 24) >> 24;

    return (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
}

int main(void) {
    uint32_t result = swap_endian_32(0x12345678);
    printf("Result: 0x%08X\n", result);
    return 0;
}