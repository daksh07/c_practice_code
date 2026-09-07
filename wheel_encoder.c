#include <stdint.h>
#include <stdio.h>

float ticks_to_distance(int32_t encoder_ticks, float wheel_diameter,
                        int32_t ticks_per_revolution) {
  float rev = (float)encoder_ticks / (float)ticks_per_revolution;
  float distance = rev * 3.14 * wheel_diameter;

  return distance;
}

int main(void) {
  float test_data[3][3] = {{180, 10, 360}, {360, 10, 360}, {720, 10, 360}};
  for (uint8_t i = 0; i < (sizeof(test_data) / sizeof(test_data[0])); i++) {
    printf(
        "Distance travelled: %f\n",
        ticks_to_distance(test_data[i][0], test_data[i][1], test_data[i][2]));
  }
  return 0;
}
