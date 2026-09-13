#include <cassert>
#include <cmath>
#include <iostream>
#include "noaa_parser.h"

int main() {
  const char json[] =
      "{\"predictions\":["
      "{\"t\":\"2026-09-12 00:00\",\"v\":\"1.2\"},"
      "{\"t\":\"2026-09-12 00:06\",\"v\":\"5.4\"},"
      "{\"t\":\"2026-09-12 00:12\",\"v\":\"-0.5\"}]}";
  TideSample samples[3]{};
  const NoaaParseResult result =
      parseNoaaPredictions(json, sizeof(json) - 1, samples, 3);
  assert(result.success);
  assert(result.count == 3);
  assert(std::fabs(samples[0].height - 1.2f) < 0.001f);
  assert(std::fabs(samples[2].height + 0.5f) < 0.001f);
  assert(result.minimum == -2.0f);
  assert(result.maximum == 6.0f);

  TideSample tooSmall[2]{};
  const NoaaParseResult overflow =
      parseNoaaPredictions(json, sizeof(json) - 1, tooSmall, 2);
  assert(overflow.overflow);
  assert(!overflow.success);

  const char malformed[] = "{\"predictions\":[{\"t\":\"bad\",\"v\":\"x\"}]}";
  const NoaaParseResult rejected =
      parseNoaaPredictions(malformed, sizeof(malformed) - 1, samples, 3);
  assert(!rejected.success);

  std::cout << "NOAA parser tests passed\n";
  return 0;
}
