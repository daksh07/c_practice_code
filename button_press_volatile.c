//#include <iomanip>
//#include <queue>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBOUNCE_DELAY_MS 50

volatile uint32_t ms_counter = 0;
volatile bool raw_state = false;
volatile uint32_t raw_change_time = 0;
volatile uint32_t raw_event_count = 0;

uint32_t confirmed_press_count = 0;
bool debounced_state = false;
uint32_t last_processed_event_count = 0;

void timer_isr_simulate(void) {
    ms_counter++;
}

void button_isr_simulate(bool new_raw_state) {
    raw_state = new_raw_state;
    printf("State change confirmed!\n");
    raw_event_count++;
    raw_change_time = ms_counter;
}

void main_loop_check(void) {
    static uint32_t last_change_time = 0;
    if((raw_event_count-last_processed_event_count)%2 != 0){
        last_change_time = raw_change_time;
    }
    printf("Last time: %d\n", last_change_time);
    if(last_change_time - raw_change_time >= DEBOUNCE_DELAY_MS){
        if((raw_event_count-last_processed_event_count)%2 != 0){
            last_change_time = raw_change_time;
            confirmed_press_count++;
            if(confirmed_press_count%2 != 0){
                debounced_state = true;
                printf("Button press confirmed\n");
                return;
            }else {
                debounced_state = false;
                printf("Button release\n");
                return;
            }
        }
    }else {
        printf("No state change.\n");
        return;
    }
}

int main(void) {
    printf("==TEST HARNESS 1==\n");
    button_isr_simulate(true);
    main_loop_check();
    printf("Time stamp 1: %d\n",ms_counter);
    for(uint32_t i = 0; i < 48; i++){
        timer_isr_simulate();
    }
    printf("Time stamp 2: %d\n", ms_counter);
    button_isr_simulate(true);
    main_loop_check();
    return 0;
}