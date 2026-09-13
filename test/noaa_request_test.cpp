#include <cassert>
#include <iostream>
#include "noaa_request.h"

int main() {
  NoaaRequest request;
  const std::string url =
      buildNoaaPredictionsUrl(request, "20260901", "20260930");
  const char expected[] =
      "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?"
      "product=predictions&application=tide-clock-esp32&begin_date=20260901"
      "&end_date=20260930&datum=MLLW&station=9437954&time_zone=lst_ldt"
      "&units=english&interval=6&format=json";
  assert(url == expected);

  request.datum = "STND";
  assert(buildNoaaPredictionsUrl(request, "a", "b").find("datum=STND") !=
         std::string::npos);
  assert(buildNoaaPredictionsUrl(request, nullptr, "b").empty());

  std::cout << "NOAA request tests passed\n";
  return 0;
}
