#include "noaa_request.h"

#include <cstdio>

namespace {
const char* kUrlFormat =
    "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?"
    "product=predictions&application=%s&begin_date=%s&end_date=%s"
    "&datum=%s&station=%s&time_zone=%s&units=%s&interval=%s&format=json";
}

std::string buildNoaaPredictionsUrl(const NoaaRequest& request,
                                    const char* beginDate,
                                    const char* endDate) {
  if (!beginDate || !endDate || !request.application || !request.datum ||
      !request.station || !request.timeZone || !request.units ||
      !request.interval) {
    return {};
  }

  const int length = std::snprintf(nullptr, 0, kUrlFormat,
                                   request.application, beginDate, endDate,
                                   request.datum, request.station,
                                   request.timeZone, request.units,
                                   request.interval);
  if (length <= 0) return {};

  std::string url(static_cast<size_t>(length), '\0');
  const int written = std::snprintf(
      url.data(), url.size() + 1, kUrlFormat, request.application, beginDate,
      endDate, request.datum, request.station, request.timeZone, request.units,
      request.interval);
  if (written != length) return {};
  return url;
}
