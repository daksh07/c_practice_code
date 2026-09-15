#include <stdint.h>
#include <stdio.h>

#define MAX_INTEGRAL 25
#define MIN_INTEGRAL -25
typedef struct {
  float kp, ki, kd;
  float integral;
  float prev_error;
} pid_controller_t;

float pid_update(pid_controller_t *pid, float setpoint, float measured_value,
                 float dt) {
  float error = setpoint - measured_value;
  // printf("Calc error %f\n", error);
  pid->integral += (error * dt);
  if (pid->integral >= MAX_INTEGRAL)
    pid->integral = MAX_INTEGRAL;
  if (pid->integral <= MIN_INTEGRAL)
    pid->integral = MIN_INTEGRAL;
  float derivative = (error - pid->prev_error) / dt;
  // printf("Calc deriv: %f\n", derivative);
  pid->prev_error = error;

  return pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
}

int main(void) {
  pid_controller_t pid;
  pid.kp = 3;
  pid.ki = 0.02;
  pid.kd = 0.05;
  pid.integral = 0.0;
  pid.prev_error = 0.0;
  float current_value = 10.0; // start far from target
  float setpoint = 57.0;

  for (int i = 0; i < 20; i++) {
    float output = pid_update(&pid, setpoint, current_value, 0.1);
    current_value +=
        output * 0.1; // simulate the robot responding to the correction
    printf("Step %d: current=%.2f  output=%.2f\n", i, current_value, output);
  }
  return 0;
}
