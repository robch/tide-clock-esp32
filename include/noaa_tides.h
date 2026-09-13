#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>
#include "secrets.h"
#include <tide_series.h>

// Match the website: retain every six-minute prediction from the 29 local
// calendar days spanning today -14 days through today +14 days. The cache is
// explicitly allocated in PSRAM so it cannot consume display/DMA memory.
constexpr size_t MAX_TIDE_SAMPLES = 7200;
static TideSample* tideSamples = nullptr;
static size_t tideSampleCount = 0;
static bool noaaDataActive = false;
static float noaaHeightMin = NAN;
static float noaaHeightMax = NAN;
static String noaaStatus = "NOAA: not loaded";

static bool initNoaaStorage() {
  if (tideSamples) return true;
  tideSamples = static_cast<TideSample*>(heap_caps_malloc(
      MAX_TIDE_SAMPLES * sizeof(TideSample), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!tideSamples) {
    noaaStatus = "NOAA: PSRAM sample allocation failed";
    Serial.printf("%s; requested=%u bytes\n", noaaStatus.c_str(),
                  static_cast<unsigned>(MAX_TIDE_SAMPLES * sizeof(TideSample)));
    return false;
  }
  Serial.printf("NOAA: PSRAM cache ready; capacity=%u records bytes=%u\n",
                static_cast<unsigned>(MAX_TIDE_SAMPLES),
                static_cast<unsigned>(MAX_TIDE_SAMPLES * sizeof(TideSample)));
  return true;
}

static String noaaDate(time_t value) {
  struct tm local{};
  localtime_r(&value, &local);
  char result[16];
  strftime(result, sizeof(result), "%Y%m%d", &local);
  return String(result);
}

static bool parseNoaa(const String& body) {
  if (!tideSamples) {
    noaaStatus = "NOAA: sample cache unavailable";
    return false;
  }

  size_t count = 0;
  int cursor = 0;
  float minHeight = INFINITY;
  float maxHeight = -INFINITY;
  time_t previous = 0;
  bool overflow = false;

  while (true) {
    const int tKey = body.indexOf("\"t\":\"", cursor);
    if (tKey < 0) break;
    const int tStart = tKey + 5;
    const int tEnd = body.indexOf('\"', tStart);
    const int vKey = body.indexOf("\"v\":\"", tEnd);
    if (tEnd < 0 || vKey < 0) break;
    const int vStart = vKey + 5;
    const int vEnd = body.indexOf('\"', vStart);
    if (vEnd < 0) break;

    int year, month, day, hour, minute;
    const String timestamp = body.substring(tStart, tEnd);
    const String valueText = body.substring(vStart, vEnd);
    if (sscanf(timestamp.c_str(), "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) == 5) {
      struct tm local{};
      local.tm_year = year - 1900;
      local.tm_mon = month - 1;
      local.tm_mday = day;
      local.tm_hour = hour;
      local.tm_min = minute;
      local.tm_isdst = -1;
      const time_t when = mktime(&local);
      const float height = valueText.toFloat();
      if (when > 0 && isfinite(height) && (count == 0 || when > previous)) {
        if (count >= MAX_TIDE_SAMPLES) {
          overflow = true;
          break;
        }
        tideSamples[count++] = {when, height};
        previous = when;
        minHeight = min(minHeight, height);
        maxHeight = max(maxHeight, height);
      }
    }
    cursor = vEnd + 1;
  }

  if (overflow) {
    noaaStatus = String("NOAA: response exceeds cache capacity ") + MAX_TIDE_SAMPLES;
    return false;
  }
  if (count < 3 || !isfinite(minHeight) || !isfinite(maxHeight) || maxHeight <= minHeight) {
    return false;
  }

  tideSampleCount = count;
  noaaHeightMin = floorf(minHeight / 2.0f) * 2.0f;
  noaaHeightMax = ceilf(maxHeight / 2.0f) * 2.0f;
  return true;
}

static bool fetchNoaaDatum(const char* datum) {
  const time_t now = time(nullptr);
  const String begin = noaaDate(now - 14 * 24 * 3600);
  const String end = noaaDate(now + 14 * 24 * 3600);
  const String url = String("https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?product=predictions&application=tide-clock-esp32&begin_date=") + begin + "&end_date=" + end + "&datum=" + datum + "&station=9437954&time_zone=lst_ldt&units=english&interval=6&format=json";
  Serial.printf("NOAA: datum=%s range=%s..%s\n", datum, begin.c_str(), end.c_str());

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) { noaaStatus = "NOAA: HTTP begin failed"; return false; }
  http.setTimeout(30000);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    noaaStatus = String("NOAA: HTTP status ") + status;
    http.end();
    return false;
  }
  const String body = http.getString();
  http.end();
  if (!parseNoaa(body)) {
    if (!noaaStatus.startsWith("NOAA:")) noaaStatus = "NOAA: parser rejected response";
    noaaStatus += String("; bytes=") + body.length();
    return false;
  }
  return true;
}

static void fetchNoaaPredictions() {
  noaaDataActive = false;
  tideSampleCount = 0;
  noaaHeightMin = NAN;
  noaaHeightMax = NAN;
  noaaStatus = "NOAA: requesting station 9437954";
  if (!initNoaaStorage()) return;
  if (WiFi.status() != WL_CONNECTED) {
    noaaStatus = "NOAA: unavailable; Wi-Fi disconnected";
    return;
  }
  Serial.printf("%s\n", noaaStatus.c_str());
  noaaDataActive = fetchNoaaDatum("MLLW") || fetchNoaaDatum("STND");
  if (noaaDataActive) {
    noaaStatus = String("NOAA LIVE: station=9437954 samples=") + tideSampleCount;
    Serial.printf("%s first=%lu last=%lu range=%.2f..%.2f ft\n", noaaStatus.c_str(),
                  static_cast<unsigned long>(tideSamples[0].when),
                  static_cast<unsigned long>(tideSamples[tideSampleCount - 1].when),
                  noaaHeightMin, noaaHeightMax);
  } else {
    tideSampleCount = 0;
    noaaStatus = String("NOAA UNAVAILABLE: ") + noaaStatus;
    Serial.printf("%s\n", noaaStatus.c_str());
  }
}

static float noaaHeightAt(time_t target) {
  if (!noaaDataActive) return NAN;
  const TideSeries series(tideSamples, tideSampleCount);
  return series.heightAt(target);
}

static float hoursFromNow(time_t value, time_t now) {
  return static_cast<float>(value - now) / 3600.0f;
}
