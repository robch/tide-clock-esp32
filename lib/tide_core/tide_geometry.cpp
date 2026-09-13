#include "tide_geometry.h"

#include <algorithm>
#include <cmath>

ClockPoint polar(float angle, float radius, float centerX, float centerY) {
  return {centerX + std::sin(angle) * radius,
          centerY - std::cos(angle) * radius};
}

float heightRadius(float height, float minimum, float maximum, float clockRadius) {
  if (maximum <= minimum) return clockRadius * 0.5f;
  const float normalized = std::clamp(
      (height - minimum) / (maximum - minimum), 0.0f, 1.0f);
  return clockRadius * (0.90f - normalized * 0.80f);
}
