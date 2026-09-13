#include <cassert>
#include <cmath>
#include <iostream>
#include "tide_geometry.h"

namespace {
void assertNear(float actual, float expected) {
  assert(std::fabs(actual - expected) < 0.0001f);
}

void test_cardinal_points() {
  const float pi = 3.14159265358979323846f;
  ClockPoint top = polar(0.0f, 10.0f, 100.0f, 100.0f);
  ClockPoint right = polar(pi / 2.0f, 10.0f, 100.0f, 100.0f);
  ClockPoint bottom = polar(pi, 10.0f, 100.0f, 100.0f);
  ClockPoint left = polar(3.0f * pi / 2.0f, 10.0f, 100.0f, 100.0f);
  assertNear(top.x, 100.0f); assertNear(top.y, 90.0f);
  assertNear(right.x, 110.0f); assertNear(right.y, 100.0f);
  assertNear(bottom.x, 100.0f); assertNear(bottom.y, 110.0f);
  assertNear(left.x, 90.0f); assertNear(left.y, 100.0f);
}

void test_height_mapping_and_clamping() {
  assertNear(heightRadius(-2.0f, -2.0f, 10.0f, 177.0f), 159.3f);
  assertNear(heightRadius(4.0f, -2.0f, 10.0f, 177.0f), 88.5f);
  assertNear(heightRadius(10.0f, -2.0f, 10.0f, 177.0f), 17.7f);
  assertNear(heightRadius(-100.0f, -2.0f, 10.0f, 177.0f), 159.3f);
  assertNear(heightRadius(100.0f, -2.0f, 10.0f, 177.0f), 17.7f);
  assertNear(heightRadius(5.0f, 4.0f, 4.0f, 177.0f), 88.5f);
}
}

int main() {
  test_cardinal_points();
  test_height_mapping_and_clamping();
  std::cout << "Tide geometry tests passed\n";
  return 0;
}
