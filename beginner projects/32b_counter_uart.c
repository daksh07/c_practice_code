#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

volatile uint32_t ms_counter = 0;
static volatile bool interrupts_disabled = false;

void disable_interrupts(void) { interrupts_disabled = true; }
void enable_interrupts(void)  { interrupts_disabled = false; }

void timer_isr_simulate(void) {
    ms_counter++;
}

uint32_t safe_read_counter(void) {
    uint32_t temp = 0;
    disable_interrupts();
    temp = ms_counter;
    enable_interrupts();
    return temp;
}

bool has_elapsed(uint32_t start_time, uint32_t duration_ms) {
    if (safe_read_counter() - start_time >= duration_ms) {
        return true;
    }
    return false;
}

int main(void) {
    printf("=== Test 1: Wraparound case ===\n");
    ms_counter = 0;
    uint32_t last_blink = UINT32_MAX;  // simulate "1ms before wraparound"

    for (int i = 0; i < 500; i++) {
        timer_isr_simulate();
        if (i == 0 || i == 1 || i == 498 || i == 499) {
            printf("iter %d: counter=%u elapsed=%d\n",
                   i, safe_read_counter(), has_elapsed(last_blink, 500));
        }
    }
    printf("Final check after 500 ticks: elapsed=%d (expect 1/true)\n\n",
           has_elapsed(last_blink, 500));

    printf("=== Test 2: Normal (non-wraparound) case ===\n");
    ms_counter = 1000;
    last_blink = safe_read_counter();

    for (int i = 0; i < 500; i++) {
        timer_isr_simulate();
        if (i == 0 || i == 1 || i == 498 || i == 499) {
            printf("iter %d: counter=%u elapsed=%d\n",
                   i, safe_read_counter(), has_elapsed(last_blink, 500));
        }
    }
    printf("Final check after 500 ticks: elapsed=%d (expect 1/true)\n\n",
           has_elapsed(last_blink, 500));

    printf("=== Test 3: Not-yet-elapsed sanity check ===\n");
    ms_counter = 5000;
    last_blink = safe_read_counter();
    for (int i = 0; i < 100; i++) timer_isr_simulate();
    printf("After 100 ticks (need 500): elapsed=%d (expect 0/false)\n",
           has_elapsed(last_blink, 500));

    return 0;
}