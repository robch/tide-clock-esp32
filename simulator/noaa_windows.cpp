#include "noaa_windows.h"

#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "noaa_parser.h"
#include "noaa_request.h"

namespace {
std::string dateString(std::time_t value) {
  std::tm local{};
  localtime_s(&local, &value);
  std::ostringstream result;
  result << std::put_time(&local, "%Y%m%d");
  return result.str();
}

bool downloadUrl(const std::string& url, std::string& body) {
  URL_COMPONENTS parts{};
  parts.dwStructSize = sizeof(parts);
  wchar_t host[256]{};
  wchar_t path[2048]{};
  parts.lpszHostName = host;
  parts.dwHostNameLength = static_cast<DWORD>(std::size(host));
  parts.lpszUrlPath = path;
  parts.dwUrlPathLength = static_cast<DWORD>(std::size(path));

  std::wstring wideUrl(url.begin(), url.end());
  if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &parts)) return false;

  HINTERNET session = WinHttpOpen(L"respi-tide-clock-sim/1.0",
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) return false;

  HINTERNET connection = WinHttpConnect(session, host, parts.nPort, 0);
  HINTERNET request = connection
      ? WinHttpOpenRequest(connection, L"GET", path, nullptr,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                           parts.nScheme == INTERNET_SCHEME_HTTPS
                               ? WINHTTP_FLAG_SECURE : 0)
      : nullptr;
  bool success = request && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS,
      0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr);

  if (success) {
    DWORD status = 0, statusSize = sizeof(status);
    success = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE |
        WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status,
        &statusSize, WINHTTP_NO_HEADER_INDEX) && status == 200;
  }

  while (success) {
    DWORD available = 0;
    if (!WinHttpQueryDataAvailable(request, &available)) { success = false; break; }
    if (available == 0) break;
    std::vector<char> chunk(available);
    DWORD received = 0;
    if (!WinHttpReadData(request, chunk.data(), available, &received)) {
      success = false; break;
    }
    body.append(chunk.data(), received);
  }

  if (request) WinHttpCloseHandle(request);
  if (connection) WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return success;
}
}

bool fetchNoaaData(SimulatorTideData& data) {
  data = {};
  const std::time_t now = std::time(nullptr);
  const std::string begin = dateString(now - 14 * 24 * 3600);
  const std::string end = dateString(now + 14 * 24 * 3600);
  NoaaRequest request;
  const std::string url = buildNoaaPredictionsUrl(request, begin.c_str(), end.c_str());
  if (url.empty()) {
    data.status = "NOAA URL construction failed";
    return false;
  }

  std::string body;
  if (!downloadUrl(url, body)) {
    data.status = "NOAA download failed";
    return false;
  }

  data.samples.resize(7200);
  const NoaaParseResult result = parseNoaaPredictions(
      body.data(), body.size(), data.samples.data(), data.samples.size());
  if (!result.success) {
    data.samples.clear();
    data.status = result.overflow ? "NOAA response too large" : "NOAA parse failed";
    return false;
  }

  data.samples.resize(result.count);
  data.minimum = result.minimum;
  data.maximum = result.maximum;
  data.live = true;
  data.status = "NOAA LIVE";
  return true;
}
