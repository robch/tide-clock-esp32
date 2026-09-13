#include <cassert>
#include <cmath>
#include <iostream>
#include "tide_series.h"

namespace {
void assertNear(float actual, float expected) {
  assert(std::isfinite(actual));
  assert(std::fabs(actual - expected) < 0.0001f);
}

void test_empty_and_out_of_range() {
  TideSeries empty;
  assert(std::isnan(empty.heightAt(100)));

  const TideSample samples[] = {{100, 2.0f}, {200, 6.0f}};
  TideSeries series(samples, 2);
  assert(std::isnan(series.heightAt(99)));
  assert(std::isnan(series.heightAt(201)));
}

void test_exact_samples_and_midpoint() {
  const TideSample samples[] = {{100, 2.0f}, {200, 6.0f}, {300, 0.0f}};
  TideSeries series(samples, 3);
  assertNear(series.heightAt(100), 2.0f);
  assertNear(series.heightAt(200), 6.0f);
  assertNear(series.heightAt(150), 4.0f);
  assertNear(series.heightAt(250), 3.0f);
}
}

int main() {
  test_empty_and_out_of_range();
  test_exact_samples_and_midpoint();
  std::cout << "TideSeries tests passed\n";
  return 0;
}
