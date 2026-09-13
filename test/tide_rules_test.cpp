#include <cassert>
#include <cmath>
#include <iostream>
#include "tide_rules.h"

namespace {
void test_window() {
  const TideSample samples[] = {{0, 1.0f}, {100, 3.0f}, {200, 1.0f}, {300, 4.0f}};
  TideSeries series(samples, 4);
  assert(hasCompleteWindow(series, 150, 100, 100));
  assert(!hasCompleteWindow(series, 150, 200, 200));
}

void test_extrema() {
  const TideSample low{100, 1.0f};
  const TideSample high{200, 5.0f};
  const TideSample nextLow{300, 2.0f};
  assert(isHighTide(low, high, nextLow));
  assert(!isLowTide(low, high, nextLow));
  assert(isLowTide(high, nextLow, {400, 6.0f}));
  assert(!isHighTide(high, nextLow, {400, 6.0f}));
}

void test_flat_samples_are_not_extrema() {
  const TideSample a{100, 3.0f};
  const TideSample b{200, 3.0f};
  const TideSample c{300, 3.0f};
  assert(!isHighTide(a, b, c));
  assert(!isLowTide(a, b, c));
}
}

int main() {
  test_window();
  test_extrema();
  test_flat_samples_are_not_extrema();
  std::cout << "Tide rules tests passed\n";
  return 0;
}
