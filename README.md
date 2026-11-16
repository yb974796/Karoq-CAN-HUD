> **Language / 語言：** [繁體中文](#traditional-chinese) ｜ [English](#english)

<a id="traditional-chinese"></a>

# Karoq CAN HUD

Škoda Karoq / Volkswagen MQB 的 **ESP32-S3 CAN HUD** 專案。以被動接收 CAN 訊號的方式，在 **640 × 172** 顯示器上呈現車速、速限標誌、ACC 設定速度、駕駛輔助狀態和原廠導航指示。

**目前穩定基準：V1.0.14.6 R1（20 RUN UI Stability）**  
**UI 基準：Karoq CAN HUD V0.6**（保留既有版面及素材）  
**Git 標籤：** [v1.0.14.6-r1](https://github.com/yb974796/Karoq-CAN-HUD/tree/v1.0.14.6-r1)

> ⚠️ 本專案僅用於資訊顯示和 CAN 訊號研究，不具備車輛控制功能。顯示的 ACC、Lane / Travel Assist 狀態不能取代原廠儀表或當成安全控制的唯一依據。

## 硬體與開發環境

| 項目 | 配置 |
|---|---|
| 開發板 | Waveshare ESP32-S3-Touch-LCD-3.49 V2 |
| Flash / PSRAM | 16 MB / 8 MB |
| HUD 解析度 | 640 × 172 |
| CAN 收發器 | TJA1051T/3 |
| CAN 控制器 | ESP32-S3 TWAI，500 kbps，Listen-only |
| CAN RX | GPIO2 |
| CAN TX | GPIO1（程式設定，但未接到車輛） |
| 開發框架 | ESP-IDF v5.5.3 |
| 圖形介面 | LVGL v9 |

## 主要功能與驗證狀態

| 功能 | 主要資料來源 | 狀態 |
|---|---|---|
| 車速 | `0x0FD` ESP_21 | 已實車確認；`0x30B` 作備援解碼 |
| ACC 設定速度、待命／啟用顯示 | `0x30C` ACC_02 | 已實車觀察部分狀態，其他組合保守處理 |
| Lane Assist / ACC / Travel 圖示 | `0x09F` EPS HCA、`0x30C` ACC | 由訊號組合判斷；不是原廠 Travel Assist 的獨立驗證 |
| 原廠導航方向與距離 | `0x17333210` / `0x17333211` BAP | 已解碼部分導航動作，未知方向不強行映射 |
| VZA 速限標誌 | `0x17332110` BAP | 試驗顯示；部分旗標語意仍未知 |
| SD CAN 記錄 | BOOT (GPIO0) 短按 | 可錄製 CSV，並記錄 NEW ID 等資訊 |

**其他已接收的候選訊號**包括 `0x0B2`（輪速）、`0x12B`（巡航按鍵）、`0x397`（車道警示）、`0x30F`（盲點監控）及 `0x366`（方向燈）。收到 CAN ID 不代表其中所有 bit 的意義已實車驗證。

### 主要 CAN 計算

- **車速：** `0x0FD`，bits 32–47，Little-endian，原始值 × 0.01 km/h。
- **ACC SET：** `0x30C`，bits 12–21，Little-endian，原始值 × 0.32 km/h；`0x3FF` 表示不可用。
- **ACC 狀態：** `0x30C` Primary bits 22–23、Display bits 61–63，僅對已驗證組合顯示狀態。
- **Lane：** `0x09F` EPS HCA Status bits 32–35，依連續資料確認，不僅以警示 LED 判斷。
- **原廠導航：** BAP Route Guidance、方向和距離；距離資料使用實車確認的 Little-endian 解析方式。

完整欄位定義請參考 [CAN ID 對照表](docs/MQB_CAN_ID_MAP.csv)、[ID Registry](docs/ID_REGISTRY.csv) 及 [ADAS 狀態矩陣](docs/ADAS_LINK_MATRIX.md)。部分文件保留早期研究階段的狀態，應與現行原始碼、實車 Log 交叉核對。

## 程式架構

```text
main/
├── core/                # 主程式／韌體版本
├── id/
│   ├── can/             # CAN 接收及分派
│   ├── decoders/        # 車速、ACC 解碼
│   ├── signals/         # MQB / ADAS 狀態
│   ├── navigation/      # 原廠導航 BAP
│   └── traffic_sign/    # VZA BAP
├── ui/
│   ├── v0_6/            # 固定的原始 V0.6 HUD UI
│   ├── bridge/          # 訊號快照與 LVGL 更新
│   └── layout/          # NAV／VZA／ACC 擴充版面
└── other/               # 按鍵、SD Logger、診斷
components/              # 板級元件
docs/                    # 文件、CAN ID、測試與版本記錄
tests/                   # 主機測試及模擬測試
sdkconfig.defaults       # ESP-IDF 預設配置
```

**介面原則：** 唯一 UI 基準為 V0.6（640×172）；保留左上駕駛輔助、左下限速標誌、中央導航、右側大型車速及 ACC SET。避免新增競爭的 LVGL 寫入者。

## Windows 編譯與燒錄

先開啟 **ESP-IDF v5.5.3 PowerShell Environment**，再執行：

```powershell
git clone https://github.com/yb974796/Karoq-CAN-HUD.git
cd Karoq-CAN-HUD
idf.py set-target esp32s3
idf.py reconfigure
idf.py build
```

確認板子 COM 埠後燒錄：

```powershell
idf.py -p COM8 flash monitor
```

`COM8` 是先前實車開發使用的埠號範例。不同電腦請依裝置管理員的 COM 埠調整。需要 ESP-IDF 相依套件與硬體配置一致，並不保證所有環境可以直接編譯成功。

## V1.0.14.6 R1 測試摘要

已完成 ESP32-S3 啟動、實車 CAN 讀取、ACC SET、部分原廠導航解析與 SD 錄製。20 RUN 的 UI 優化包含選擇性圖示更新與 ACC 短暫異常的防閃爍處理。

**待追查：** `speed_layout_mismatches` 診斷、LCD 視覺閃爍是否完全消除、部分導航方向、VZA Flags，以及尚未確認的 ADAS 狀態位元。請勿把未驗證訊號作為車輛控制依據。

## 開發安全與資料保護

- CAN 僅被動監聽；**不發送**加速、煞車、轉向或其他車輛控制命令。
- 行駛時請勿由駕駛操作接線、筆電、串列監視或燒錄程序。
- 提交 GitHub 前，應排除含 VIN、行程、車輛識別資訊的 CAN 紀錄、金鑰及秘密設定。
- 版本升級建議先在開發分支測試，再合併 `main` 並建立 Git 標籤。

## 參考資料與授權

- [Espressif ESP-IDF](https://docs.espressif.com/projects/esp-idf/)
- [LVGL](https://docs.lvgl.io/)
- [comma.ai opendbc](https://github.com/commaai/opendbc)

目前 Repository 未附正式 `LICENSE`；若需公開授權或接受貢獻，請先盤點原始碼、素材和第三方元件授權。

---

<a id="english"></a>

# English Version

## Overview

**Karoq CAN HUD** is an experimental ESP32-S3 head-up display project for the Škoda Karoq / Volkswagen MQB platform. It passively receives CAN frames and renders driving information on a **640 × 172** display: vehicle speed, speed-limit signs, ACC set speed, assistance states, and OEM navigation instructions.

**Current stable baseline:** V1.0.14.6 R1 (20 RUN UI Stability)  
**UI baseline:** Karoq CAN HUD V0.6 — original layout and assets retained  
**Version tag:** [v1.0.14.6-r1](https://github.com/yb974796/Karoq-CAN-HUD/tree/v1.0.14.6-r1)

> ⚠️ **Display and research only.** This is not a vehicle controller or a substitute for the factory instrument cluster. The ACC, Lane Assist, and Travel-style indicators reflect observed CAN signals; they must not be treated as authoritative confirmation of system engagement or driving safety.

## Hardware and Development Environment

| Item | Configuration |
|---|---|
| Board | Waveshare ESP32-S3-Touch-LCD-3.49 V2 |
| Flash / PSRAM | 16 MB / 8 MB |
| HUD display | 640 × 172 |
| CAN transceiver | TJA1051T/3 |
| CAN controller | ESP32-S3 TWAI, 500 kbps, listen-only |
| CAN RX | GPIO2 |
| CAN TX | GPIO1 (configured in firmware, not connected to the vehicle) |
| SDK | ESP-IDF v5.5.3 |
| UI framework | LVGL v9 |

## Features and Validation Status

| Feature | Data source | Status / limitation |
|---|---|---|
| Live vehicle speed | Standard CAN `0x0FD` ESP_21 | Verified with on-vehicle logs; `0x30B` provides a decoded fallback |
| ACC set speed and standby/active display | Standard CAN `0x30C` ACC_02 | Several combinations observed on the vehicle; unknown combinations are handled conservatively |
| ACC / Lane / Travel-style indicator | `0x09F` EPS HCA and `0x30C` ACC | Inferred from signal combinations; **not** independent verification of OEM Travel Assist |
| OEM navigation maneuver and distance | Extended CAN `0x17333210` / `0x17333211` BAP | Selected maneuvers and distance decoded; unverified maneuvers are not guessed |
| VZA speed-limit sign | Extended CAN `0x17332110` BAP | Trial display; the exact meaning of some flags remains unverified |
| SD CAN logging | Short press of BOOT (GPIO0) | CSV recording and NEW ID tracking |

Other candidate IDs observed in logs include `0x0B2` (wheel speed), `0x12B` (cruise buttons), `0x397` (lane warning), `0x30F` (blind spot), and `0x366` (turn signals). **Receiving an ID does not validate every bit field on this particular vehicle.**

### Key CAN Fields

- **Vehicle speed:** `0x0FD`, bits 32–47, little-endian, raw value × 0.01 km/h.
- **ACC set speed:** `0x30C`, bits 12–21, little-endian, raw value × 0.32 km/h; `0x3FF` means unavailable.
- **ACC status:** `0x30C`, Primary bits 22–23 and Display bits 61–63; only validated combinations are reflected in the UI.
- **Lane status:** `0x09F`, EPS HCA Status bits 32–35; confirmation uses consecutive observations, not just a warning LED.
- **OEM navigation:** BAP route-guidance, maneuver, and distance fields; the observed distance representation is decoded as little-endian.

See [MQB CAN ID map](docs/MQB_CAN_ID_MAP.csv), [ID Registry](docs/ID_REGISTRY.csv), and [ADAS state matrix](docs/ADAS_LINK_MATRIX.md). Some documents preserve earlier research-stage observations; cross-check them against the current source code and vehicle logs.

## Repository Structure

```text
main/
├── core/                # Startup, firmware information
├── id/
│   ├── can/             # CAN receive and dispatch
│   ├── decoders/        # Speed and ACC decoding
│   ├── signals/         # MQB and ADAS state handling
│   ├── navigation/      # OEM navigation BAP
│   └── traffic_sign/    # VZA traffic-sign BAP
├── ui/
│   ├── v0_6/            # Original V0.6 HUD UI and assets
│   ├── bridge/          # Signal snapshots to LVGL
│   └── layout/          # Navigation, VZA, and ACC layout
└── other/               # Buttons, SD logger, diagnostics
components/              # Board-level components
docs/                    # Documentation and CAN reference
tests/                   # Host and simulation tests
sdkconfig.defaults       # ESP-IDF defaults
```

**UI design rule:** V0.6 (640 × 172) remains the single layout baseline: assistance status at upper left, speed-limit sign at lower left, navigation in the center, large speed at right, and ACC set speed at lower right. Avoid competing LVGL writers.

## Build and Flash on Windows

Open an **ESP-IDF v5.5.3 PowerShell Environment**:

```powershell
git clone https://github.com/yb974796/Karoq-CAN-HUD.git
cd Karoq-CAN-HUD
idf.py set-target esp32s3
idf.py reconfigure
idf.py build
```

After confirming the board's serial port, flash and monitor:

```powershell
idf.py -p COM8 flash monitor
```

`COM8` is an example port from development; adjust it for your Windows device. A compatible ESP-IDF dependency and hardware setup is required. Successful builds are not guaranteed on every machine without configuration checks.

## V1.0.14.6 R1 Test Summary

ESP32-S3 startup, live CAN reception, ACC set-speed display, partial OEM navigation decoding, and SD recording have been exercised on the vehicle. The 20 RUN changes include selective LVGL asset updates and short-dropout handling to reduce ACC visual flicker.

**Open issues:** `speed_layout_mismatches` diagnostics, confirmation that LCD flicker is fully resolved, unmapped navigation maneuvers, VZA flags, and remaining unverified ADAS signal fields. Unknown signals must not be used for vehicle control or safety-critical assumptions.

## Safety and Data Privacy

- CAN is **listen-only**; this firmware does **not transmit** acceleration, braking, steering, or other vehicle control frames.
- Drivers must not handle wiring, debugging tools, serial monitoring, or firmware flashing while driving.
- Keep raw logs containing VINs, location history, or other vehicle-identifying data—and any secrets or private keys—out of public commits.
- Test changes on a development branch before merging into `main` and assigning a release tag.

## References and License

- [Espressif ESP-IDF](https://docs.espressif.com/projects/esp-idf/)
- [LVGL](https://docs.lvgl.io/)
- [comma.ai opendbc](https://github.com/commaai/opendbc)

No project-wide `LICENSE` file has been selected yet. Review copyright and license obligations for project code, assets, and third-party dependencies before offering redistribution rights.
