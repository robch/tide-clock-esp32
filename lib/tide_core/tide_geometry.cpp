#include "tide_geometry.h"

#include <algorithm>
#include <cmath>

ClockPoint polar(float angle, float radius, float centerX, float centerY) {
  return {centerX + std::sin(angle) * radius,
          centerY - std::cos(angle) * radius};
}

float timeAngle(time_t timestamp) {
  struct tm local{};
#if defined(_WIN32)
  if (localtime_s(&local, &timestamp) != 0) return 0.0f;
#else
  if (localtime_r(&timestamp, &local) == nullptr) return 0.0f;
#endif
  const float hour12 = (local.tm_hour % 12) +
                       local.tm_min / 60.0f +
                       local.tm_sec / 3600.0f;
  constexpr float kTau = 6.28318530717958647692f;
  return hour12 * kTau / 12.0f;
}

float heightRadius(float height, float minimum, float maximum, float clockRadius) {
  if (maximum <= minimum) return clockRadius * 0.5f;
  const float normalized = std::clamp(
      (height - minimum) / (maximum - minimum), 0.0f, 1.0f);
  return clockRadius * (0.90f - normalized * 0.80f);
}
