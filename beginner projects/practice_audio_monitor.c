#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

void main(){
    int8_t year = 0;
    int8_t age = 0;
    printf("Enter your year of birth\n");
    scanf("%d", year);

    printf("You are % years old \n", (2026 - year));
}