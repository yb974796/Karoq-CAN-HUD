# Karoq CAN HUD V1.0.8 — Original V0.6 Native UI / 10RUN

This is an **upgrade package for your working V1.0.7 ESP-IDF project**, not a complete independently verified firmware `bin`. It includes the real `Karoq_CAN_HUD_V0_6_Patch(1).zip` UI source and original images, but uses the Waveshare V2 LCD BSP, LVGL components, CAN/SD drivers, sdkconfig and hardware settings from your installed V1.0.7 project. It NEVER modifies or overwrites the V1.0.7 project. An existing destination is a hard stop.

## 改動內容

- 唯一畫面：`Karoq CAN HUD V0.6`，640×172 黑底、左側 64×31 VAG 駕駛輔助圖、右側 70×104 圖片字形速度、狀態文字。**V0.6 本來就沒有導航和道路限速標誌**，不可宣稱它們目前會顯示。
- **修正 V1.0.7 的關鍵 Bug：車速是三個 `lv_image`，不是 `lv_label`。** 改用原生 `karoq_hud_ui_set_state()`，一次接收速度/輔助狀態。移除舊 Label 速度橋接、舊 ADAS Image Binding Timer、Guard 三種互相覆寫路徑。
- 透過 CMake compile definition `KAROQ_HUD_DEMO_AUTORUN=0` **從開機即關閉 Demo Timer**。不必用 Guard 等到第一次收到 CAN 再壓制 `88`。
- 原本 V0.6 的 4 張圖片、所有 0–9 字形、速度布局、解析度保持原樣。唯一 UI `.c` 程式碼微調：`char speed[4]` 改 `char speed[6]`，解決 GCC `-Wformat-truncation`，不影響 0..299 畫面。
- 開機 LVGL 100ms Timer 從已存在的 `karoq_speed_get()` + `karoq_mqb_assist_runtime_get()` 讀取 snapshot。C runtime 讀取與原始 UI 作為不同 translation units，避免兩組 `KAROQ_ASSIST_*` enum 同名衝突。
- 舊 `0x0FD` 主車速、`0x30B` 備援、`0x30C` ACC 顯示、`0x120` TSK_06 的保守 ACC 連動與 SDMMC／BOOT 記錄保留；**不更動 TWAI 接收邏輯、不發送 CAN**。
- 避免誤顯示 Lane/Travel：`LDW_02 / LH_EPS_03 / HCA_01` 候選訊號仍記錄，但在尚未驗證 Karoq 車道置中真正 Active 訊號前，UI 仍灰色。只有 TSK_06 + ACC_02 連續有效時允許 ACC_ONLY。

## 安裝（ESP-IDF 5.5.3 PowerShell）

1. 將 ZIP 解壓至 `D:\Karoq_HUD\Karoq_HUD_V1_0_8_V06_Native_10RUN`。
2. 先確認你的 **V1.0.7 專案可以正常編譯**，原檔位於 `D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_7`。本包不含完整 Waveshare BSP 的來源工程，**必須從該現有專案複製**。
3. 在 IDF PowerShell 執行：

```powershell
cd D:\Karoq_HUD\Karoq_HUD_V1_0_8_V06_Native_10RUN
.\Upgrade_V1_0_8.ps1 -SourceProject 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_7' -Destination 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_8'

cd D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_8
python D:\Karoq_HUD\Karoq_HUD_V1_0_8_V06_Native_10RUN\tools\verify_v108.py --project .

idf.py reconfigure
idf.py build
idf.py -p COM8 flash monitor
```

If your installed V1.0.7 is at a different path, replace `-SourceProject`. If it cannot be found or is an inconsistent V1.0.7 version, upgrade STOPS with a clear error and leaves original source untouched. **Do not use `idf.py fullclean` until after saving a known-good build and sdkconfig.**

## 預期 Log（尚非實機結果）

```text
KAROQ_UI: V1.0.8 native V0.6 HUD link attached: images not Labels, speed+ADAS single writer
KAROQ_UI: V1.0.8 NATIVE V0.6: real CAN speed glyphs LIVE; HUD demo disabled
KAROQ_CAN: STATUS ... SPEED=... src=ESP_21/0x0FD ... lost=0 ... bus_err=0
KAROQ_SD: ... BOOT tap toggles SD ...
```

A logger issue must not stop CAN reception; `queue_dropped`, `io_errors`, `tracker_full` must be verified through real SD test. Before putting it in the car, check the screen at 0 km/h while safely stationary. For slow-motion checks, use a passenger to read the screen; no driver interaction with PC.

## 10RUN 檢查

`python tests/ten_run.py` executes 10 independent host checks including *compiling and linking the actual uploaded V0.6 UI source* against LVGL 9 mocked headers, MQB/TSK state regression, SD ID tracker regression, CMake script-mode, upgrade preservation, refusal to overwrite and ten deterministic upgrade dry-runs. **These are host simulations, not ESP-IDF hardware or an actual FAT32 SD card run.** Results saved under `docs/10RUN_RESULTS.json`.

## 功能的確定程度

- **UI**: this release contains the original V0.6 visual assets and sprites, but not an actual ESP32 screen photograph. Original preview is `docs/V0.6_original_preview.png`.
- **Speed / passive CAN RX**: V1.0.7 historical real car logs showed 0x0FD works; V1.0.8 updated UI must be checked on the car.
- **ACC / Lane**: software decoder metadata exists, but TSK_06 and camera-side CAN IDs were absent from some previous Comfort CAN capture sessions. Never infer the lane/Travel icon green from missing signals.
- **SD**: V1.0.4 host-tested logger is preserved, but actual high-frequency FAT32 write and rapid taps need fresh hardware testing.
- **Build**: host C syntax/logic tests passed. Full IDF build is NOT performed in this environment, since there is no complete on-device Waveshare ESP-IDF project and BSP here.
