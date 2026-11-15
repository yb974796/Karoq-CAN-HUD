# Karoq CAN HUD V0.1 — Portable Architecture + Host Tests

**NOT a Waveshare factory firmware build. NOT a flashable BIN.**

This design pack gives independent vehicle-specific files, generic 11/29-bit CAN unsigned Intel-signal decoder, normalized HUD state, basic source arbitration, host tests, and reserved modules for VW MQB CAN-K, BAP and iPhone navigation. It can be integrated into the official Waveshare V2 `10_LVGL_V9_Test` project after a successful baseline build.

## Tested now

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Passes `HOST_CORE_TEST_PASS` on a normal C++17 host environment (test uses an OpenDBC-based synthetic sample at 80.0 km/h; it is **not real car data**).

## Official V2 baseline build on Windows (user machine)

- Official page: https://docs.waveshare.net/ESP32-S3-Touch-LCD-3.49/ESP-IDF/
- Official V2 source: https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49-V2
- Set up ESP-IDF v5.5.3 in VS Code.
- Start with V2 `10_LVGL_V9_Test` project, NOT the V1 project.
- In ESP-IDF terminal at the official demo's project root:

```
idf.py set-target esp32s3
idf.py build
```

Only after original Flash backup, proceed to `idf.py -p COMx flash monitor` with the actual COM port. Physical flashing is out of scope of this package.

## Design boundaries

- `core` contains portable CAN rule decoding and state types. ESP-IDF CAN driver and LVGL live outside the core.
- `profiles/skoda_karoq_2019/can_comfort.csv` intentionally has no fabricated CAN-K IDs. Use your Karoq raw CAN-K logs before adding rules.
- `can_extended_reference.csv` has DBC demo definitions for testing, not confirmed present on CAN-K.
- `bap_routes.json` only contains tentative VZA routing references. No working BAP reassembly/decoder is implied.
- `navigation_sources.json` defines `oem_bap` and optional `ios_companion`; generic BLE pairing does NOT access Apple Maps or Google Maps navigation state.
- Vehicle CAN is **listen-only** by design, no TX features in this pack.
- Never accept a stale speed limit as a current road sign; source confidence and timeout are part of each state.

## Modular future tree

- main controller: app_main/boot/health/watchdog
- board abstraction: Waveshare V2 LCD/backlight/SD/RTC/touch
- CAN transport: TWAI RX and raw logger
- decoder: physical-bus-specific DBC rules, CRC/counter checks
- BAP: dedicated framing, service decoding and reassembly
- navigation: OEM BAP adapter, dedicated iOS companion adapter
- sign recognition: OEM A5/VZA, optional map-estimate as separate labeled fallback
- UI: LVGL render-only tasks; no raw CAN parsing inside display callbacks

## Build status

Portable C++ tests can run on host. **ESP-IDF 5.5.3 and Waveshare V2 demo source were unavailable in the execution environment for an actual firmware build.** Do not call this a validated ESP32 binary.

## Main controller / demo

`core/main_controller.hpp` owns the message routing (CAN → vehicle state, OEM/iPhone → navigation mux, A5 → sign mux) but has no ESP32 dependencies. This is the main application layer required by the modular design. Run:

```
./build/hud_demo profiles/skoda_karoq_2019/can_extended_reference.csv
```

Sample output is synthetic and demonstrates a speed of 80 km/h, a navigation right turn in 1700 m, and a camera-sign placeholder of 60 km/h. This is strictly simulated data, not an active Apple/Google Maps bridge. The example phone message schema is in `docs/ios_navigation_protocol.json`.
