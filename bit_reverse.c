/* Reverse the order of bits in a single byte — e.g. 0b10110001 becomes 0b10001101. 
 * Comes up in things like LSB-first vs MSB-first serial protocols (SPI/UART bit ordering mismatches), 
 * and is a good complement to your endian-swap exercise, 
 * but at the bit level instead of the byte level. */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

uint8_t reverse_bits(uint8_t byte){
	uint8_t bit1 = byte & 1;
	uint8_t bit2 = (byte & 1 << 1) >> 1;
	uint8_t bit3 = (byte & 1 << 2) >> 2;
	uint8_t bit4 = (byte & 1 << 3) >> 3;
	uint8_t bit5 = (byte & 1 << 4) >> 4;
	uint8_t bit6 = (byte & 1 << 5) >> 5;
	uint8_t bit7 = (byte & 1 << 6) >> 6;
	uint8_t bit8 = (byte & 1 << 7) >> 7;

	uint8_t new_byte = (bit1 << 7) + (bit2 << 6) + (bit3 << 5) + (bit4 << 4) + (bit5 << 3) + (bit6 << 2) + (bit7 << 1) + bit8;

	return new_byte;
}

int main(){
	printf("Enter a number\n");
	int input;
	scanf("%d", &input);
	uint8_t byte = (uint8_t)input;
	byte = reverse_bits(byte);
	printf("Reversed byte: %d\n", byte);

	return 0;
}

