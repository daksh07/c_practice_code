#include <math.h>
#include <stdio.h>

typedef struct {
  float x;
  float y;
  float heading; // in radians
} robot_pose_t;

void update_odometry(robot_pose_t *pose, float left_distance,
                     float right_distance, float wheel_base) {
  float avg_distance = (left_distance + right_distance) / 2;
  printf("Average distance: %f\n", avg_distance);
  float delta_head = (right_distance - left_distance) / wheel_base;
  printf("Delta head: %f\n", delta_head);
  pose->x += avg_distance * cosf(pose->heading);
  pose->y += avg_distance * sinf(pose->heading);
  pose->heading += delta_head;
  printf("Heading calc: %f\n", pose->heading);
  while (pose->heading > M_PI) {
    pose->heading -= 2 * M_PI;
  }
  while (pose->heading < -M_PI) {
    pose->heading += 2 * M_PI;
  }
}

int main(void) {
  robot_pose_t pos;
  pos.x = 0.0;
  pos.y = 0.0;
  pos.heading = 0.0;
  update_odometry(&pos, 40, 30, 5);
  printf("Robot heading: %f\n", pos.heading * (180 / M_PI));
  update_odometry(&pos, 40, 15, 5);
  printf("Robot heading: %f\n", pos.heading * (180 / M_PI));
  update_odometry(&pos, 20, 60, 5);
  printf("Robot heading: %f\n", pos.heading * (180 / M_PI));
  return 0;
}
