#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <math.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include "lcd_bsp.h"
#include "FT3168.h"
#include "secrets.h"
#include "noaa_tides.h"
#include <tide_geometry.h>

namespace {
constexpr int W = 466;
constexpr int H = 466;
constexpr int CX = W / 2;
constexpr int CY = H / 2;
constexpr float TAU = 6.28318530718f;
constexpr float CLOCK_RADIUS = 177.0f;
constexpr float DEFAULT_MIN_HEIGHT = -2.0f;
constexpr float DEFAULT_MAX_HEIGHT = 10.0f;
constexpr time_t SIX_MINUTES = 6 * 60;

lv_obj_t* canvas = nullptr;
lv_color_t* canvasBuffer = nullptr;

lv_color_t color(uint8_t r, uint8_t g, uint8_t b) { return lv_color_make(r, g, b); }

lv_color_t blend(lv_color_t foreground, lv_color_t background, float amount) {
  amount = constrain(amount, 0.0f, 1.0f);
  return lv_color_mix(foreground, background,
                      static_cast<lv_opa_t>(lroundf(amount * LV_OPA_COVER)));
}

lv_point_t polar(float angle, float radius) {
  const ClockPoint point = ::polar(angle, radius, CX, CY);
  return {static_cast<lv_coord_t>(lroundf(point.x)),
          static_cast<lv_coord_t>(lroundf(point.y))};
}

void line(lv_point_t a, lv_point_t b, lv_color_t c, uint8_t width = 1) {
  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color = c;
  dsc.width = width;
  dsc.round_start = true;
  dsc.round_end = true;
  lv_point_t points[2] = {a, b};
  lv_canvas_draw_line(canvas, points, 2, &dsc);
}

void circle(lv_point_t center, int radius, lv_color_t c, uint8_t width = 1) {
  lv_draw_arc_dsc_t dsc;
  lv_draw_arc_dsc_init(&dsc);
  dsc.color = c;
  dsc.width = width;
  lv_canvas_draw_arc(canvas, center.x, center.y, radius, 0, 360, &dsc);
}

void filledCircle(lv_point_t center, int radius, lv_color_t c) {
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = c;
  dsc.bg_opa = LV_OPA_COVER;
  dsc.radius = LV_RADIUS_CIRCLE;
  dsc.border_opa = LV_OPA_TRANSP;
  lv_canvas_draw_rect(canvas, center.x - radius, center.y - radius,
                      radius * 2 + 1, radius * 2 + 1, &dsc);
}

void filledQuad(const lv_point_t (&points)[4], lv_color_t c, lv_opa_t opacity) {
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = c;
  dsc.bg_opa = opacity;
  dsc.border_opa = LV_OPA_TRANSP;
  lv_canvas_draw_polygon(canvas, points, 4, &dsc);
}

float heightRadius(float height) {
  const float minimum = noaaDataActive ? noaaHeightMin : DEFAULT_MIN_HEIGHT;
  const float maximum = noaaDataActive ? noaaHeightMax : DEFAULT_MAX_HEIGHT;
  // Exact website mapping: low=90% of R, high=10% of R.
  return ::heightRadius(height, minimum, maximum, CLOCK_RADIUS);
}

float timeAngle(time_t sampleTime) {
  struct tm local{};
  localtime_r(&sampleTime, &local);
  const float hour12 = (local.tm_hour % 12) + local.tm_min / 60.0f + local.tm_sec / 3600.0f;
  return hour12 * TAU / 12.0f;
}

void drawTextCentered(lv_point_t p, int width, const char* text, lv_color_t textColor) {
  lv_draw_label_dsc_t dsc;
  lv_draw_label_dsc_init(&dsc);
  dsc.color = textColor;
  dsc.font = &lv_font_montserrat_14;
  dsc.align = LV_TEXT_ALIGN_CENTER;
  lv_canvas_draw_text(canvas, p.x - width / 2, p.y - 8, width, &dsc, text);
}

void drawNumerals(lv_color_t numeralColor) {
  static const char* labels[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
  for (int hour = 0; hour < 12; ++hour) {
    drawTextCentered(polar(hour * TAU / 12.0f, CLOCK_RADIUS + 22.0f), 28, labels[hour], numeralColor);
  }
}

void drawNoDataMessage() {
  drawTextCentered({CX, CY - 10}, 140, "NO TIDE DATA", color(255, 105, 105));
}

void drawStatusLabel(bool live) {
  drawTextCentered({CX, H - 18}, 140, live ? "NOAA LIVE" : "NOAA OFFLINE",
                   live ? color(93, 177, 192) : color(210, 95, 95));
}

void drawBaseFace(lv_color_t pale, lv_color_t grid) {
  circle({CX, CY}, static_cast<int>(CLOCK_RADIUS), color(127, 168, 189), 2);
  circle({CX, CY}, static_cast<int>(CLOCK_RADIUS * 0.10f), grid, 1);
  circle({CX, CY}, static_cast<int>(CLOCK_RADIUS * 0.90f), grid, 1);

  const float minimum = noaaDataActive ? noaaHeightMin : DEFAULT_MIN_HEIGHT;
  const float maximum = noaaDataActive ? noaaHeightMax : DEFAULT_MAX_HEIGHT;
  for (float feet = minimum; feet <= maximum + 0.01f; feet += 2.0f) {
    circle({CX, CY}, static_cast<int>(lroundf(heightRadius(feet))), grid, 1);
  }

  // Website order: tide graphics are drawn after this base, then ticks and
  // numerals are redrawn on top below.
  (void)pale;
}

void drawTicksAndNumerals(lv_color_t pale, lv_color_t grid) {
  for (int minute = 0; minute < 60; ++minute) {
    const float angle = minute * TAU / 60.0f;
    const bool hourTick = minute % 5 == 0;
    const float inner = CLOCK_RADIUS - (hourTick ? 12.0f : 6.0f);
    line(polar(angle, inner), polar(angle, CLOCK_RADIUS + 4.0f),
         hourTick ? pale : grid, hourTick ? 2 : 1);
  }
  drawNumerals(pale);
}

void drawMainTideRing(time_t now, lv_color_t cyan, lv_color_t fillColor) {
  const time_t end = now + 12 * 3600;
  time_t aTime = now;
  float aHeight = noaaHeightAt(aTime);
  if (!isfinite(aHeight)) return;

  while (aTime < end) {
    const time_t bTime = min(aTime + SIX_MINUTES, end);
    const float bHeight = noaaHeightAt(bTime);
    if (!isfinite(bHeight)) return;

    const lv_point_t farA = polar(timeAngle(aTime), CLOCK_RADIUS);
    const lv_point_t farB = polar(timeAngle(bTime), CLOCK_RADIUS);
    const lv_point_t curveA = polar(timeAngle(aTime), heightRadius(aHeight));
    const lv_point_t curveB = polar(timeAngle(bTime), heightRadius(bHeight));
    const lv_point_t quad[4] = {farA, farB, curveB, curveA};
    filledQuad(quad, fillColor, static_cast<lv_opa_t>(90));
    line(curveA, curveB, cyan, 3);

    aTime = bTime;
    aHeight = bHeight;
  }
}

void drawContextTrace(time_t start, time_t end, bool fadeIn,
                      lv_color_t trace, lv_color_t background) {
  time_t aTime = start;
  float aHeight = noaaHeightAt(aTime);
  if (!isfinite(aHeight)) return;
  const float span = static_cast<float>(end - start);

  while (aTime < end) {
    const time_t bTime = min(aTime + SIX_MINUTES, end);
    const float bHeight = noaaHeightAt(bTime);
    if (!isfinite(bHeight)) return;
    const float midpoint = static_cast<float>((aTime - start) + (bTime - start)) * 0.5f / span;
    const float visibility = fadeIn ? midpoint : 1.0f - midpoint;
    line(polar(timeAngle(aTime), heightRadius(aHeight)),
         polar(timeAngle(bTime), heightRadius(bHeight)),
         blend(trace, background, 0.85f * visibility), 2);
    aTime = bTime;
    aHeight = bHeight;
  }
}

void drawHighLowMarkers(time_t now, lv_color_t yellow, lv_color_t background) {
  const time_t end = now + 12 * 3600;
  for (size_t i = 1; i + 1 < tideSampleCount; ++i) {
    const TideSample& sample = tideSamples[i];
    if (sample.when < now || sample.when > end) continue;
    const bool high = sample.height >= tideSamples[i - 1].height &&
                      sample.height >= tideSamples[i + 1].height;
    const bool low = sample.height <= tideSamples[i - 1].height &&
                     sample.height <= tideSamples[i + 1].height;
    if ((!high && !low) ||
        (sample.height == tideSamples[i - 1].height && sample.height == tideSamples[i + 1].height)) continue;

    const float angle = timeAngle(sample.when);
    const float radius = heightRadius(sample.height);
    const lv_point_t point = polar(angle, radius);
    if (high) {
      filledCircle(point, 5, yellow);
      circle(point, 5, background, 1);
    } else {
      filledCircle(point, 5, background);
      circle(point, 5, yellow, 2);
    }

    char label[12];
    snprintf(label, sizeof(label), "%.1f%c", sample.height, high ? 'H' : 'L');
    drawTextCentered(polar(angle, radius + (high ? -17.0f : 17.0f)), 48, label, yellow);
  }
}

void drawClockFace() {
  if (!canvas) return;
  const lv_color_t background = color(8, 17, 32);
  const lv_color_t pale = color(220, 240, 248);
  const lv_color_t grid = color(38, 73, 99);
  const lv_color_t cyan = color(79, 214, 255);
  const lv_color_t fillColor = color(45, 170, 214);
  const lv_color_t yellow = color(255, 209, 102);
  const lv_color_t context = color(150, 150, 155);
  const lv_color_t red = color(255, 107, 107);

  lv_canvas_fill_bg(canvas, background, LV_OPA_COVER);
  drawBaseFace(pale, grid);

  const time_t now = time(nullptr);
  if (noaaDataActive) {
    const bool completeWindow = isfinite(noaaHeightAt(now - 6 * 3600)) &&
                                isfinite(noaaHeightAt(now + 18 * 3600));
    if (completeWindow) {
      drawMainTideRing(now, cyan, fillColor);
      drawHighLowMarkers(now, yellow, background);
      drawTicksAndNumerals(pale, grid);
      drawContextTrace(now - 6 * 3600, now, true, context, background);
      drawContextTrace(now + 12 * 3600, now + 18 * 3600, false, context, background);

      const float currentHeight = noaaHeightAt(now);
      const lv_point_t current = polar(timeAngle(now), heightRadius(currentHeight));
      circle(current, 7, cyan, 2);
      filledCircle(current, 3, cyan);
    } else {
      drawTicksAndNumerals(pale, grid);
      drawNoDataMessage();
    }
  } else {
    drawTicksAndNumerals(pale, grid);
    drawNoDataMessage();
  }

  struct tm localNow{};
  localtime_r(&now, &localNow);
  const float secondAngle = localNow.tm_sec * TAU / 60.0f;
  const float minuteAngle = (localNow.tm_min + localNow.tm_sec / 60.0f) * TAU / 60.0f;
  const float hourAngle = ((localNow.tm_hour % 12) + localNow.tm_min / 60.0f +
                           localNow.tm_sec / 3600.0f) * TAU / 12.0f;
  line({CX, CY}, polar(hourAngle, CLOCK_RADIUS * 0.50f), pale, 5);
  line({CX, CY}, polar(minuteAngle, CLOCK_RADIUS * 0.75f), pale, 3);
  line(polar(secondAngle, -CLOCK_RADIUS * 0.12f),
       polar(secondAngle, CLOCK_RADIUS * 0.85f), red, 2);
  filledCircle({CX, CY}, 6, color(224, 82, 82));
  circle({CX, CY}, 6, pale, 1);
  filledCircle({CX, CY}, 3, red);

  drawStatusLabel(noaaDataActive);
  lv_obj_invalidate(canvas);
}
} // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("BOOT: website-faithful NOAA tide clock");
  Serial.println("NOAA CO-OPS datagetter is public; no API key is required.");

  // Reserve all display/DMA resources before allocating the large NOAA cache.
  Touch_Init();
  lcd_lvgl_Init();
  canvasBuffer = static_cast<lv_color_t*>(heap_caps_malloc(
      W * H * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!canvasBuffer) {
    Serial.println("ERROR: canvas PSRAM allocation failed");
    while (true) delay(1000);
  }
  canvas = lv_canvas_create(lv_scr_act());
  lv_canvas_set_buffer(canvas, canvasBuffer, W, H, LV_IMG_CF_TRUE_COLOR);
  drawClockFace();

  if (!initNoaaStorage()) {
    drawClockFace();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Wi-Fi: connecting");
  const uint32_t wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi connected; IP=%s RSSI=%d channel=%d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
    configTzTime("PST8PDT,M3.2.0,M11.1.0", "pool.ntp.org", "time.nist.gov");
    Serial.println("NTP: synchronization requested");
    const uint32_t ntpStart = millis();
    while (time(nullptr) < 1700000000 && millis() - ntpStart < 15000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    if (time(nullptr) >= 1700000000) {
      Serial.printf("NTP: valid epoch=%lu; requesting NOAA data\n",
                    static_cast<unsigned long>(time(nullptr)));
      fetchNoaaPredictions();
    } else {
      Serial.println("NTP: synchronization failed; tide visualization disabled");
    }
  } else {
    Serial.println("NOAA: unavailable because Wi-Fi did not connect; tide visualization disabled");
  }

  drawClockFace();
  if (noaaDataActive) Serial.println("PASS: complete NOAA tide dataset rendered");
  else Serial.println("STATUS: NO TIDE DATA; synthetic fallback is disabled");
}

void loop() {
  static uint32_t lastDraw = 0;
  static uint32_t lastFetch = 0;
  static uint32_t lastStatus = 0;
  if (millis() - lastDraw >= 1000) {
    lastDraw = millis();
    drawClockFace();
  }
  if (millis() - lastFetch >= 10UL * 60UL * 1000UL) {
    lastFetch = millis();
    fetchNoaaPredictions();
    drawClockFace();
  }
  if (millis() - lastStatus >= 5000) {
    lastStatus = millis();
    if (noaaDataActive) {
      Serial.printf("STATUS: NOAA LIVE station=9437954 samples=%u first=%lu last=%lu min=%.2f max=%.2f current=%.2f\n",
                    static_cast<unsigned>(tideSampleCount),
                    static_cast<unsigned long>(tideSamples[0].when),
                    static_cast<unsigned long>(tideSamples[tideSampleCount - 1].when),
                    noaaHeightMin, noaaHeightMax, noaaHeightAt(time(nullptr)));
    } else {
      Serial.println("STATUS: NO TIDE DATA; no synthetic data is being rendered");
    }
  }
  lv_timer_handler();
  delay(10);
}
