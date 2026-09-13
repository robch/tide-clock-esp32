#pragma once

struct ClockPoint {
  float x;
  float y;
};

// Clock coordinates use 12 o'clock as angle zero and increase clockwise.
ClockPoint polar(float angle, float radius, float centerX, float centerY);

// Maps low tide near the outside of the ring and high tide near the center.
float heightRadius(float height, float minimum, float maximum, float clockRadius);
