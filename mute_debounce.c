#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBOUNCE_DELAY_MS 50

bool debounce_button(bool raw_reading, uint32_t current_time_ms) {
    // TODO: You'll need some persistent state between calls to track:
    // - the last raw reading seen
    // - the timestamp when that raw reading last changed
    // - the last confirmed (debounced) stable state
    //
    // Logic:
    // 1. If raw_reading differs from the last raw reading seen, reset the timer
    //    (record current_time_ms as "when it last changed")
    // 2. If raw_reading has stayed the same since that timestamp for at least
    //    DEBOUNCE_DELAY_MS, update and return the debounced state
    // 3. Otherwise, return the previous debounced state unchanged (not stable yet)
    static uint32_t last_time_ms = 0;
    static bool prev_raw = false;
    static bool lst_state = false;
    if(raw_reading != prev_raw){
        prev_raw = raw_reading;
        last_time_ms = current_time_ms;
    }
    if(current_time_ms - last_time_ms > DEBOUNCE_DELAY_MS){
        lst_state = raw_reading;
    }
    return lst_state;
}

int main(void) {

    if(debounce_button(true, 100)){printf("Button pressed!\n");};
    if(debounce_button(true, 120)){printf("Button pressed!\n");};
    if(debounce_button(true, 151)){printf("Button pressed!\n");};
    if(!(debounce_button(false, 160))){printf("Button released!\n");};
    if(!(debounce_button(false, 170))){printf("Button released!\n");};
    if(!(debounce_button(false, 211))){printf("Button released!\n");};
    // TODO: simulate a sequence of (raw_reading, current_time_ms) calls
    // that includes bounce noise, and print the debounced output each time
    // to confirm it only settles once, after the delay

    // --- Bounce noise sequence ---
    // Real contact bounce: rapid true/false/true/false flicker within
    // the debounce window, before finally settling on 'true'.
    printf("\n--- Bounce test ---\n");
    printf("t=300 raw=true  -> %s\n", debounce_button(true, 300)  ? "true" : "false");
    printf("t=305 raw=false -> %s\n", debounce_button(false, 305) ? "true" : "false"); // bounce
    printf("t=308 raw=true  -> %s\n", debounce_button(true, 308)  ? "true" : "false"); // bounce
    printf("t=312 raw=false -> %s\n", debounce_button(false, 312) ? "true" : "false"); // bounce
    printf("t=363 raw=true  -> %s\n", debounce_button(true, 363)  ? "true" : "false"); // stable, past 50ms since t=31
    printf("t=420 raw=true  -> %s\n", debounce_button(true, 420)  ? "true" : "false"); // held stable, should confirm
    
    return 0;
}