#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

bool is_watchdog_starved(uint32_t last_kick_time, uint32_t current_time, uint32_t timeout_ms) {
    if(current_time - last_kick_time >= timeout_ms){
        printf("Watchdog needs service\n");
        return true;
    }
    printf("Watchdog Okay\n");
    return false;
}

int main(void) {
    is_watchdog_starved(1001, 2000, 1000);
    is_watchdog_starved(1000, 2000, 250);
    is_watchdog_starved(UINT32_MAX - 100, 200, 400);
    
    return 0;
}