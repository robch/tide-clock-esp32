#include "tide_rules.h"

#include <cmath>

bool hasCompleteWindow(const TideSeries& series, time_t now,
                       time_t before, time_t after) {
  if (series.count() < 2) return false;
  return std::isfinite(series.heightAt(now - before)) &&
         std::isfinite(series.heightAt(now + after));
}

bool isHighTide(const TideSample& previous, const TideSample& current,
                const TideSample& next) {
  return current.height >= previous.height &&
         current.height >= next.height &&
         !(current.height == previous.height && current.height == next.height);
}

bool isLowTide(const TideSample& previous, const TideSample& current,
               const TideSample& next) {
  return current.height <= previous.height &&
         current.height <= next.height &&
         !(current.height == previous.height && current.height == next.height);
}
