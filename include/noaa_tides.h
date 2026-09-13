#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>
#include "secrets.h"
#include <noaa_parser.h>
#include <noaa_request.h>
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

  const NoaaParseResult result = parseNoaaPredictions(
      body.c_str(), body.length(), tideSamples, MAX_TIDE_SAMPLES);
  if (result.overflow) {
    noaaStatus = String("NOAA: response exceeds cache capacity ") + MAX_TIDE_SAMPLES;
    return false;
  }
  if (!result.success) return false;

  tideSampleCount = result.count;
  noaaHeightMin = result.minimum;
  noaaHeightMax = result.maximum;
  return true;
}

static bool fetchNoaaDatum(const char* datum) {
  const time_t now = time(nullptr);
  const String begin = noaaDate(now - 14 * 24 * 3600);
  const String end = noaaDate(now + 14 * 24 * 3600);
  NoaaRequest request;
  request.datum = datum;
  const std::string url =
      buildNoaaPredictionsUrl(request, begin.c_str(), end.c_str());
  if (url.empty()) {
    noaaStatus = "NOAA: URL construction failed";
    return false;
  }
  Serial.printf("NOAA: datum=%s range=%s..%s\n", datum, begin.c_str(), end.c_str());

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  const String arduinoUrl(url.c_str());
  if (!http.begin(client, arduinoUrl)) { noaaStatus = "NOAA: HTTP begin failed"; return false; }
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
