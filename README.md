# ES3C28P — ESP32-S3 with 2.8" display and LVGL

**PlatformIO** project (Arduino framework) for the **ES3C28P** board: ESP32-S3, 16 MB flash, OPI PSRAM. The UI is built with **LVGL** on top of **TFT_eSPI**; touch is over I²C, with **WiFi** (on-screen SSID / password) and the async web stack as declared in the current configuration.

> **Board target:** In `platformio.ini` the target is `esp32-s3-devkitc-1` — a standard ESP32-S3 profile for this hardware. TFT and touch pins are defined in `include/TFT_User_Setup.h` and `include/touch.h`.

## Hardware (as defined in the repository)

| Component     | Spec / part |
|--------------|------------|
| SoC          | ESP32-S3, PSRAM, 16 MB flash (partition table `default_16MB.csv`) |
| Display      | ILI9341, 240×320, SPI |
| Touch        | FT6336, I²C: SCL `15`, SDA `16`, INT `17`, RST `18` |
| SPI (TFT_eSPI) | MISO `13`, MOSI `11`, SCLK `12`, CS `10`, DC `46` |
| Backlight   | `TFT_BL` = GPIO `45` (active `HIGH`) |

Wiring or vendor wiki links can be added later; for now, pins are only what is in `include/`.

## Software features (current)

- LVGL display via `lv_tft_espi_create()`.
- Touch: read coordinates, mapped to display rotation.
- WiFi: **App WiFi** in the home drawer (SSID scan, password, Connect / Disconnect); status bar uses `WiFi.status()` for the Wi-Fi icon. Global **Keyboard** module (LVGL keyboard on the top layer, Montserrat 14).
- On-screen FPS label (rough draw performance).
- Serial console at **115200** baud (see `platformio.ini`).

## Tooling

- [PlatformIO](https://platformio.org/) (CLI or integration with VS Code / Cursor).
- USB cable to the ESP32-S3.

## Build and flash

```text
pio run
pio run -t upload
pio device monitor
```

**TFT user setup:** `TFT_ESPI_USER_SETUP_PATH` points to `include/TFT_User_Setup.h` (in `build_flags` in `platformio.ini`).

**Dependencies (`lib_deps`):** TFT_eSPI, LVGL, AsyncTCP, ESPAsyncWebServer.

## Project layout (common paths)

- `src/main.cpp` — `setup` / `loop`, LVGL, touch, WiFi.
- `include/` — TFT config, touch, WiFi, shared UI helpers.
