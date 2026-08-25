#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define POLYNOMIAL 0x07

uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0;

    for(uint8_t i = 0; i<len; i++){
        crc = crc ^ data[i];
        uint8_t topbit = (crc & (1<<7))>>7;
        for (uint8_t j = 0; j < 8; j++){
            if (topbit){
                crc = (crc << 1) ^ POLYNOMIAL;
            }
            else {
                crc = crc << 1;
            }
            topbit = (crc & (1<<7))>>7;
        }
    }
    return crc;
}

bool crc8_verify(const uint8_t *data, size_t len) {
    if(crc8(data, len) == 0){
        printf("Data verified successfully!\n");
        return true;
    }
    else {
        printf("Corrupt data\n");
        return false;
    }

}

int main(void) {
    uint8_t payload1[4] = {0x01, 0x02, 0x03, 0x48};
    uint8_t payload2[5] = {0xAA, 0xAB, 0xCC, 0xDD, 0x69};
    uint8_t payload3[2] = {0xFF, 0xF3};
    printf("===TEST CASE 1====\n");
    crc8_verify(payload1, 4);
    printf("===TEST CASE 2====\n");
    crc8_verify(payload2, 5);
    printf("===TEST CASE 3====\n");
    crc8_verify(payload3, 2);
    return 0;
}