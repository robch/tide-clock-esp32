#pragma once

#include <time.h>

struct ClockPoint {
  float x;
  float y;
};

// Clock coordinates use 12 o'clock as angle zero and increase clockwise.
ClockPoint polar(float angle, float radius, float centerX, float centerY);

// Maps a local wall-clock timestamp using the device convention: 12 o'clock
// is zero angle and angles increase clockwise.
float timeAngle(time_t timestamp);

// Maps low tide near the outside of the ring and high tide near the center.
float heightRadius(float height, float minimum, float maximum, float clockRadius);
