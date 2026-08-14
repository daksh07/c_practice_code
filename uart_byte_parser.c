#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define START_BYTE 0xAA
#define MAX_PAYLOAD 16

typedef enum {
    WAIT_START,
    WAIT_CMD,
    WAIT_LEN,
    WAIT_PAYLOAD,
    WAIT_CHECKSUM
} parser_state_t;

typedef struct {
    parser_state_t state;
    uint8_t cmd;
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD];
    uint8_t payload_idx;
    uint8_t checksum_calc;
} uart_parser_t;

void parser_init(uart_parser_t *parser) {
    parser->state = WAIT_START;
    parser->len = 0;
    for(uint8_t i = 0; i < MAX_PAYLOAD; i++){
        parser->payload[i] = 0;
    }
    parser->payload_idx = 0;
    parser->checksum_calc = 0;
}

bool parser_feed_byte(uart_parser_t *parser, uint8_t byte) {
    //static uint8_t run_checksum = 0;
    //static uint32_t size = 0;
    switch (parser->state) {
        case WAIT_START:
            if (byte == START_BYTE)
            {
                parser->state = WAIT_CMD;
                printf("Start Byte received\n");
            }
            else{
                printf("Wrong start byte: %d\n",byte);
                printf("Start byte mismatch. Garbage data, start again!\n");
            }
        break;
        case WAIT_CMD:
            parser->cmd = byte;
            parser->checksum_calc = parser->checksum_calc ^ byte;
            parser->state = WAIT_LEN;
            printf("Command received.\n");
        break;
        case WAIT_LEN:
            if (byte == 0) {
                parser->len = byte;
                parser->checksum_calc = parser->checksum_calc ^ byte;
                parser->state = WAIT_CHECKSUM;
                printf("Zero-length payload, skipping to checksum.\n");
            }
            else if(byte <= MAX_PAYLOAD){
                parser->len = byte;
                parser->checksum_calc = parser->checksum_calc ^ byte;
                parser->state = WAIT_PAYLOAD;
                printf("Length received and verfied.\n");
            }
            else {
                parser->checksum_calc = 0;
                parser->state = WAIT_START;
                printf("Wrong data length. Garbage data, start again!\n");
            }
        break;
        case (WAIT_PAYLOAD):
            if(parser->payload_idx < (parser->len)-1){
                parser->payload[parser->payload_idx] = byte;
                parser->checksum_calc = parser->checksum_calc ^ byte;
                parser->payload_idx++;
            }else if (parser->payload_idx == (parser->len)-1) {
                parser->payload[parser->payload_idx] = byte;
                parser->checksum_calc = parser->checksum_calc ^ byte;
                parser->payload_idx = 0;
                for (uint8_t i = 0; i < parser->len; i++) {
                    printf("Received and stored: %d\n",parser->payload[i]);
                }
                parser->state = WAIT_CHECKSUM;
            }
        break;
        case (WAIT_CHECKSUM):{
            if (byte == parser->checksum_calc){
                printf("Checksum matches\n");
                parser->checksum_calc = 0;
                parser->state = WAIT_START;
                return true;
            }
            else {
                printf("Checksum doesnt match. Start again.\n");
            }
            parser->checksum_calc = 0;
            parser->state = WAIT_START;
        }
        break;
    }
    return false;
}

int main(void) {
    uint8_t test_load[3][5] = {
        {0xAA, 3, 1, 34, 32},   // Valid frame, LEN=1: CMD=3, LEN=1, payload=[34], checksum = 3^1^34 = 32
        {0xAA, 5, 0, 5, 0x00},  // Valid frame, LEN=0 (no payload): CMD=5, LEN=0, checksum = 5^0 = 5 -- trailing 0x00 is padding, should be ignored/rejected as a stray non-start byte
        {0xAB, 3, 1, 34, 32}    // Invalid: wrong start byte (0xAB instead of 0xAA), whole row should be rejected in WAIT_START
    };
    //uint8_t size1 = sizeof(test_load)/sizeof(test_load[0]);
    //printf("size of main array: %d\n",size1);
    uart_parser_t parser;
    parser_init(&parser);
    uint8_t i = 0;
    for (i = 0; i < sizeof(test_load)/sizeof(test_load[0]); i++){
        for(uint8_t j = 0; j < sizeof(test_load[0])/sizeof(test_load[0][0]); j++){
            //printf("test load sent %d\n",test_load[i][j]);
            parser_feed_byte(&parser, test_load[i][j]);
        }
    }
    // your test harness
    return 0;
}