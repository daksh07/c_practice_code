#include <math.h>
#include <stdio.h>

#define EARTH_RADIUS 6371

float haversine_distance(float lat1, float lon1, float lat2, float lon2) {
  float lat1_rad = lat1 * M_PI / 180;
  float lat2_rad = lat2 * M_PI / 180;
  float lon1_rad = lon1 * M_PI / 180;
  float lon2_rad = lon2 * M_PI / 180;
  float del_lat = lat2_rad - lat1_rad;
  float del_lon = lon2_rad - lon1_rad;

  float a = (sin(del_lat / 2) * sin(del_lat / 2)) +
            cos(lat1_rad) * cos(lat2_rad) * sin(del_lon / 2) * sin(del_lon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  float distance = EARTH_RADIUS * c;

  return distance;
}

float bearing_to_waypoint(float current_lat, float current_lon,
                          float target_lat, float target_lon) {
  float current_lat_rad = current_lat * M_PI / 180;
  float current_lon_rad = current_lon * M_PI / 180;
  float target_lat_rad = target_lat * M_PI / 180;
  float target_lon_rad = target_lon * M_PI / 180;
  float delta_lon_rad = target_lon_rad - current_lon_rad;

  float heading = atan2(sin(delta_lon_rad) * cos(target_lat_rad),
                        cos(current_lat_rad) * sin(target_lat_rad) -
                            sin(current_lat_rad) * cos(target_lat_rad) *
                                cos(delta_lon_rad));
  float bearing = heading * (180 / M_PI);
  if (bearing < 0)
    bearing += 360;

  return bearing;
}

int main(void) {
  struct {
    float lat1, lon1, lat2, lon2;
    const char *label;
    float expect_dist;
    float expect_bearing;
  } tests[] = {
      {51.5074, -0.1278, 48.8566, 2.3522, "London -> Paris", 344.0, 156.0},
      {48.8566, 2.3522, 51.5074, -0.1278, "Paris -> London", 344.0, 337.0},
      {40.7128, -74.0060, 34.0522, -118.2437, "New York -> LA", 3936.0, -1},
      {19.0761, 72.8774, 28.61000, 77.23000, "Mumbai -> Delhi", 1149.0,
       21.7}, // no clean reference bearing, just sanity
      {10.0, 20.0, 20.0, 20.0, "Due north", 0.0, 0.0},
      {10.0, 20.0, 10.0, 30.0, "Due east", 0.0, 90.0},
      {20.0, 20.0, 10.0, 20.0, "Due south", 0.0, 180.0},
      {10.0, 30.0, 10.0, 20.0, "Due west", 0.0, 270.0},
      {51.5074, -0.1278, 51.5074, -0.1278, "Same point", 0.0,
       -1}, // bearing undefined at zero distance
  };

  int n = sizeof(tests) / sizeof(tests[0]);
  for (int i = 0; i < n; i++) {
    float dist = haversine_distance(tests[i].lat1, tests[i].lon1, tests[i].lat2,
                                    tests[i].lon2);
    float bear = bearing_to_waypoint(tests[i].lat1, tests[i].lon1,
                                     tests[i].lat2, tests[i].lon2);

    printf("%-16s distance=%8.2f km", tests[i].label, dist);
    if (tests[i].expect_dist > 0)
      printf(" (expect ~%.0f)", tests[i].expect_dist);
    printf("   bearing=%6.1f deg", bear);
    if (tests[i].expect_bearing >= 0)
      printf(" (expect ~%.0f)", tests[i].expect_bearing);
    printf("\n");
  }

  return 0;
}
