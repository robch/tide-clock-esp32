#include "noaa_parser.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace {
const char* findWithin(const char* begin, const char* end, const char* token) {
  const size_t tokenLength = std::strlen(token);
  if (tokenLength == 0 || static_cast<size_t>(end - begin) < tokenLength) return nullptr;
  for (const char* cursor = begin; cursor + tokenLength <= end; ++cursor) {
    if (std::memcmp(cursor, token, tokenLength) == 0) return cursor;
  }
  return nullptr;
}

bool copyQuoted(const char* start, const char* end, char* output, size_t capacity,
                const char*& next) {
  if (start >= end || capacity == 0) return false;
  const char* closing = static_cast<const char*>(std::memchr(start, '"', end - start));
  if (!closing || static_cast<size_t>(closing - start) >= capacity) return false;
  std::memcpy(output, start, static_cast<size_t>(closing - start));
  output[closing - start] = '\0';
  next = closing + 1;
  return true;
}
}

NoaaParseResult parseNoaaPredictions(const char* json, size_t length,
                                     TideSample* output, size_t capacity) {
  NoaaParseResult result;
  if (!json || !output || capacity == 0) return result;

  const char* begin = json;
  const char* end = json + length;
  const char* cursor = begin;
  time_t previous = 0;
  float minimum = INFINITY;
  float maximum = -INFINITY;

  while (cursor < end) {
    const char* tKey = findWithin(cursor, end, "\"t\":\"");
    if (!tKey) break;
    const char* tStart = tKey + 5;
    char timestamp[32];
    const char* afterTimestamp = nullptr;
    if (!copyQuoted(tStart, end, timestamp, sizeof(timestamp), afterTimestamp)) break;

    const char* vKey = findWithin(afterTimestamp, end, "\"v\":\"");
    if (!vKey) break;
    const char* vStart = vKey + 5;
    char valueText[24];
    const char* afterValue = nullptr;
    if (!copyQuoted(vStart, end, valueText, sizeof(valueText), afterValue)) break;

    int year, month, day, hour, minute;
    if (std::sscanf(timestamp, "%d-%d-%d %d:%d",
                    &year, &month, &day, &hour, &minute) == 5) {
      struct tm local{};
      local.tm_year = year - 1900;
      local.tm_mon = month - 1;
      local.tm_mday = day;
      local.tm_hour = hour;
      local.tm_min = minute;
      local.tm_isdst = -1;
      const time_t when = std::mktime(&local);
      char* valueEnd = nullptr;
      const float height = std::strtof(valueText, &valueEnd);
      if (when > 0 && valueEnd != valueText && std::isfinite(height) &&
          (result.count == 0 || when > previous)) {
        if (result.count >= capacity) {
          result.overflow = true;
          break;
        }
        output[result.count++] = {when, height};
        previous = when;
        minimum = std::min(minimum, height);
        maximum = std::max(maximum, height);
      }
    }
    cursor = afterValue;
  }

  if (result.count < 3 || !std::isfinite(minimum) ||
      !std::isfinite(maximum) || maximum <= minimum) {
    return result;
  }
  result.minimum = std::floor(minimum / 2.0f) * 2.0f;
  result.maximum = std::ceil(maximum / 2.0f) * 2.0f;
  result.success = !result.overflow;
  return result;
}
