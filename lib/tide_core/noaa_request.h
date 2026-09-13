#pragma once

#include <string>

struct NoaaRequest {
  const char* station = "9437954";
  const char* application = "tide-clock-esp32";
  const char* datum = "MLLW";
  const char* timeZone = "lst_ldt";
  const char* units = "english";
  const char* interval = "6";
};

// Build the exact NOAA predictions URL. Returns an empty string for invalid
// inputs or if formatting fails.
std::string buildNoaaPredictionsUrl(const NoaaRequest& request,
                                    const char* beginDate,
                                    const char* endDate);
