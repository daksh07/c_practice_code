//#include <cstdio>
//#include <regex>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define LOCK_CODE 14

/*Write a function that monitors a door sensor and controls a door lock. 
If the door is closed and the correct code is entered, 
unlock the door for 5 seconds. 
If the door is forced open without a code, 
trigger an alarm and set a security flag.*/

volatile bool door_open = false;
int lock_code = 0;
char door_cmd[5] = "Close";
volatile bool correct_code = false;
volatile bool alarm = false;
bool check_code();
void lock_code_isr();

bool door_sensor_isr(){
    int8_t timer = 6;
    if((!strcmp(&door_cmd, "Open")) && check_code()){
        printf("Opening door....\n");
        door_open = true;
        for (int32_t i = 0; i < 5000; i++){
            if(i%1000 == 0){
                timer--;
                printf("Door closing in %d secs\n", timer);
            }
        };
        door_open = false;
        printf("Door locked. \n");
    }
    if (!(strcmp(&door_cmd, "Open")) && !(check_code())) {
        alarm = true;
        printf("Error! Wrong code.\n");
        printf("WARNING: BEEP. BEEP. BEEP.\n");
    }
    if (!(strcmp(&door_cmd, "Open")) != 1){
        printf("Illegal command. Try again. \n");
        lock_code_isr();
        door_sensor_isr();
    }
    return false;
}

void lock_code_isr(){
    printf("Enter the code:\n");
    scanf("%d",&lock_code);
    printf("Enter Door command\n");
    scanf("%s", &door_cmd);
}

bool check_code(){
    if (lock_code == LOCK_CODE){
        printf("Code accepted!\n");
        correct_code = true;
        return true;
    }else {
        return false;
    }
}

int main(){
    lock_code_isr();
    //check_code();
    door_sensor_isr();
    return 0;
}
