#include "tide_series.h"

#include <math.h>

float TideSeries::heightAt(time_t target) const {
  if (!samples_ || count_ < 2) return NAN;
  if (target < samples_[0].when || target > samples_[count_ - 1].when) return NAN;

  // Find the first sample at or after target. This preserves the original
  // noaaHeightAt() binary-search behavior while keeping the algorithm portable.
  size_t low = 1;
  size_t high = count_ - 1;
  while (low < high) {
    const size_t mid = low + (high - low) / 2;
    if (samples_[mid].when < target) low = mid + 1;
    else high = mid;
  }

  const size_t i = low;
  const float span = static_cast<float>(samples_[i].when - samples_[i - 1].when);
  const float fraction = span > 0
      ? static_cast<float>(target - samples_[i - 1].when) / span
      : 0.0f;
  return samples_[i - 1].height +
         (samples_[i].height - samples_[i - 1].height) * fraction;
}
