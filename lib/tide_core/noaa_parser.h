#pragma once

#include <stddef.h>
#include "tide_series.h"

struct NoaaParseResult {
  bool success = false;
  bool overflow = false;
  size_t count = 0;
  float minimum = 0.0f;
  float maximum = 0.0f;
};

// Parse NOAA predictions into caller-owned storage. The parser does not own
// or allocate the output buffer and does not depend on Arduino or networking.
NoaaParseResult parseNoaaPredictions(const char* json, size_t length,
                                     TideSample* output, size_t capacity);
