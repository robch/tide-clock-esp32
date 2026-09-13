#pragma once

#include <string>
#include <vector>
#include "tide_series.h"

struct SimulatorTideData {
  std::vector<TideSample> samples;
  float minimum = 0.0f;
  float maximum = 0.0f;
  bool live = false;
  std::string status;
};

// Synchronously fetch the shared NOAA request and parse it into simulator data.
bool fetchNoaaData(SimulatorTideData& data);
