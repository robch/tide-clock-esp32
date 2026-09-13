#pragma once

#include <time.h>
#include "tide_series.h"

// True when the series can provide every value needed for the clock's
// historical and future context traces.
bool hasCompleteWindow(const TideSeries& series, time_t now,
                       time_t before, time_t after);

// Classify a sample using its immediate neighbors. Equal neighboring values
// are not extrema, matching the tide-clock display behavior.
bool isHighTide(const TideSample& previous, const TideSample& current,
                const TideSample& next);
bool isLowTide(const TideSample& previous, const TideSample& current,
               const TideSample& next);
