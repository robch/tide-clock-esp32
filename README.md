# Respi: ESP32-S3 AMOLED Wi-Fi/mDNS demo

`respi` is an independent copy of the working Waveshare LVGL + touch + Wi-Fi web demo. It adds multicast DNS (mDNS), so the device can be reached by name instead of a DHCP address.

## Device URL

```text
http://respi.local/
```

The device also prints its numeric DHCP address over serial. The `.local` name should work when the computer is on the same LAN and multicast traffic is allowed.

## Features

- Waveshare 1.43-inch AMOLED display
- SH8601/CO5300 panel detection through the reference BSP
- FT3168 touchscreen
- LVGL UI
- Wi-Fi using local ignored `src/secrets.h`
- HTTP server on port 80
- Browser action that changes the on-device button/status
- mDNS hostname `respi`
- Advertised `_http._tcp` service

## Build and upload

```bat
cd C:\src\platio\waveshare-tide-clock-prototype
pio run
pio run --target upload
pio device monitor --port COM3 --baud 115200
```

Close the monitor before uploading. The current machine uses COM3. On another machine, discover the port with `pio device list`, then replace `COM3` in the monitor command. The `upload_port` and `monitor_port` settings in `platformio.ini` are machine-specific; update them locally when the device uses a different port.

## Naming

The hostname was chosen as `respi`, a short name inspired by the round ESP32 display and pi/3.14 wordplay. The mDNS URL is `http://respi.local/`.

mDNS is local-network discovery, not internet exposure. It does not provide authentication or encryption; anyone allowed onto the same LAN may be able to reach the HTTP server.

## Dependency versions

- LVGL: 8.4.0, vendored under `lib/lvgl`
- Waveshare AMOLED BSP: local project copy under `lib/waveshare_amoled`
- ESP32 Arduino platform: pioarduino platform referenced by `platformio.ini`

The local dependency copies are intentional so the v0.1 build remains reproducible. LVGL and the Waveshare BSP are checked into this repository, but the pioarduino platform URL in `platformio.ini` currently tracks the repository default rather than a pinned release or commit. A fully reproducible future build should pin that platform to a known version or commit.

The Waveshare BSP is a local copy of the board-reference implementation used by this project. The repository does not currently record an upstream BSP URL, release, or commit for it; capture that provenance if the BSP is updated later.

The LVGL source directory also contains some untracked duplicate files with names such as `README 2.md` and `lvgl 2.h`. These appear to be accidental copy artifacts and are not part of the build; remove them from the working tree after confirming they are not needed.


## Credentials

`src/secrets.h` contains the local Wi-Fi credentials and is excluded by the workspace `.gitignore`. Never commit that file or place credentials in tracked source.
