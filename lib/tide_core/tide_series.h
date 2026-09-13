#pragma once

#include <stddef.h>
#include <time.h>

// Platform-neutral view of a prediction sample. The ESP32 stores these in
// PSRAM; desktop callers may store them anywhere with the same layout.
struct TideSample {
  time_t when;
  float height;
};

// A non-owning, chronologically sorted tide prediction series.
// The caller owns the sample buffer and controls its lifetime.
class TideSeries {
 public:
  TideSeries() = default;
  TideSeries(const TideSample* samples, size_t count)
      : samples_(samples), count_(count) {}

  void reset(const TideSample* samples, size_t count) {
    samples_ = samples;
    count_ = count;
  }

  size_t count() const { return count_; }
  const TideSample* samples() const { return samples_; }

  // Returns an interpolated height, or NAN outside the series' time range.
  // Samples must be chronologically sorted, as NOAA's response is.
  float heightAt(time_t target) const;

 private:
  const TideSample* samples_ = nullptr;
  size_t count_ = 0;
};
