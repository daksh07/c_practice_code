

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { ENC_NONE, ENC_CW, ENC_CCW } encoder_result_t;

void binprintf(int v) {
  unsigned int mask = 1 << ((sizeof(int) << 3) - 1);
  while (mask) {
    printf("%d", (v & mask ? 1 : 0));
    mask >>= 1;
  }
}

encoder_result_t decode_rotary_encoder(uint8_t current_ab,
                                       uint8_t previous_ab) {
  uint8_t data = (previous_ab << 2) + current_ab;
  // printf("Combined data: %d\n", data);
  switch (data) {
  case 0b0001:
    return ENC_CW;
    break;
  case 0b0111:
    return ENC_CW;
    break;
  case 0b1110:
    return ENC_CW;
    break;
  case 0b1000:
    return ENC_CW;
    break;
  case 0b0010:
    return ENC_CCW;
    break;
  case 0b1011:
    return ENC_CCW;
    break;
  case 0b1101:
    return ENC_CCW;
    break;
  case 0b0100:
    return ENC_CCW;
    break;
  default:
    return ENC_NONE;
    break;
  }
}

int main(void) {
  uint8_t test_data[12] = {0b00, 0b01, 0b01, 0b11, 0b11, 0b01,
                           0b01, 0b00, 0b11, 0b11, 0b00, 0b11};
  for (uint8_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i += 2) {
    encoder_result_t state =
        decode_rotary_encoder(test_data[i + 1], test_data[i]);
    if (state == ENC_CW) {
      printf("Encoder move clockwise. \n");
    } else if (state == ENC_CCW) {
      printf("Encoder move anti clockwise. \n");
    } else {
      printf("Invalid encoder signal. \n");
    }
  }
  return 0;
}
