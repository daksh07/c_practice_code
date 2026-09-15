#include <stdio.h>
#include <stdint.h>

void print_binary(uint32_t value, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
    }
    printf("\n");
}

void set_register_bits(volatile uint32_t *reg, uint32_t mask, uint32_t value) {
    uint32_t temp = *reg;
    temp = temp & ~(mask);
    value = value & mask;
    *reg = temp | value;
}

int main(void) {
    uint32_t reg = 0b10101101;
    printf("Reg val: ");
    print_binary(reg, 8);
    uint32_t mask = 0b00111000;
    printf("Mask: ");
    print_binary(mask, 8);
    uint32_t value = 0b01011001;
    printf("Value: ");
    print_binary(value, 8);
    set_register_bits(&reg, mask, value);
    printf("New reg val: ");
    print_binary(reg, 8);
    return 0;
}