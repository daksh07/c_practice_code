#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#define POLYNOMIAL 0x07

uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0;

    for (uint8_t i = 0; i < len; i++){
        crc = crc ^ data[i];
        uint8_t topbit = (crc & (1 << 7))>>7;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (topbit){
                crc = (crc << 1) ^ POLYNOMIAL;
            }
            else {
                crc = crc << 1;
            }
            topbit = (crc & (1 << 7))>>7;
        }  
    }
    return crc;
}

int main(void) {
    const char *str = "123456789";
    uint8_t crc = crc8((const uint8_t *)str, strlen(str));
    //char data = "123456789";
    //uint8_t crc = crc8(&data, 1);
    printf("CRC: %d\n", crc);
    return 0;
}