#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void compute_wheel_speeds(float linear_velocity, float angular_velocity,
                          float wheel_base, float *left_speed,
                          float *right_speed) {
  *left_speed = linear_velocity - (angular_velocity * wheel_base / 2);
  *right_speed = linear_velocity + (angular_velocity * wheel_base / 2);
  printf("Left speed: %f\n", *left_speed);
  printf("Right speed: %f\n", *right_speed);
}

int main(void) {
  float left_speed = 0;
  float right_speed = 0;
  float test_data[3][3] = {{20, 15, 10}, {20, -15, 10}, {20, 0, 10}};
  for (uint8_t i = 0; i < (sizeof(test_data) / sizeof(test_data[0])); i++) {
    compute_wheel_speeds(test_data[i][0], test_data[i][1], test_data[i][2],
                         &left_speed, &right_speed);
    if (left_speed > right_speed) {
      printf("Turning right. \n");
    } else if (left_speed < right_speed) {
      printf("Turning left. \n");
    } else {
      printf("Moving straight");
    }
  }
  return 0;
}
