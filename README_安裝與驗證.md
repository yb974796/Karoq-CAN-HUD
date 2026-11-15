# Karoq CAN HUD V1.0.2 — 官方 UI：CAN sniffer 分支 V0.6

**ESP-IDF 5.5.3 / LVGL 9.5 / Waveshare ESP32-S3-Touch-LCD-3.49 V2，640×172。**

這是可執行的 **V1.0.1 → V1.0.2 遷移套件**，不是可在空白硬碟直接燒錄的預編譯 `.bin`。**假設 V1.0.1 專案已使用你確認的 V0.6 原始畫面；工具只保證原檔不變，無法在缺少 V0.6 原始碼的情況下辨識或重新還原某個不同 UI 版本。**原因是你原版 `karoq_hud.c`、圖示和 BSP 原始碼只存在你電腦上；套件從原始專案保留並帶入它們，絕不偽造 V0.6 圖示。

## 新版重點

- **介面僅使用「分支 · 專案分析 CAN sniffer / Karoq CAN HUD V0.6」**。真實 UI 與 assets 原檔 SHA-256 驗證完全一致；不得附加 V0.8 overlay。
- 保留已實車驗證：500kbps Comfort CAN、TWAI Listen Only、GPIO2 RX、GPIO1 TX 未接線；`0x0FD` 即時車速、`0x30B` 備援、`0x30C` ACC 原始監控；CAN 調試 `lost/overrun/bus_err`。
- 正式路徑：`main/core`、`main/id`、`main/ui/v0_6`、`main/ui/bridge`、`main/ui/guard`、`main/other`，每個模組可獨立擴充。
- 專案名稱統一 `karoq_can_hud_v1_0_2`，COM8 啟動新增 `KAROQ_CORE FW v1.0.2 ...`。
- **Guard 暫時保留**：真實 V0.6 `karoq_hud.c` 未曾上傳；已有實車 Log 證明 Guard 攔截舊 Demo 寫入。停掉 Guard 會再次跳 `88`。套件提供 `audit_v06_speed_demo.py` 唯讀報告，供下一版精確移除舊 Demo 更新來源。

## 0. 條件

請已完成 V1.0.1 實車測試，原專案路徑為 `D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_Integrated`，內有：

- `main/core/main.c`
- `main/ui/legacy/karoq_hud.c`、`karoq_hud_assets.c`、所有其他 V0.6 素材
- `main/id` 內已成功解碼的車速與 ACC 程式
- 原版 BSP、`sdkconfig`、`managed_components` 等

## 1. 產生 V1.0.2 獨立專案

將 ZIP 解壓到 `D:\Karoq_HUD`。在 **ESP-IDF 5.5.3 PowerShell** 輸入：

```powershell
cd D:\Karoq_HUD\Karoq_HUD_V1_0_2_V06_UI_Release
.\Upgrade_V1_0_2.ps1 -SourceProject 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_Integrated' -Destination 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_2'
```

升級器會拒絕覆蓋已有的目標目錄，也不會修改來源專案。若升級失敗，會清理自己建立的暫存目錄，不更動 V1.0.1。

## 2. 驗證、編譯與燒錄

```powershell
cd D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_2
.\Verify_V1_0_2.ps1 -Project 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_2'

idf.py reconfigure
idf.py build
idf.py -p COM8 flash monitor
```

預期 COM8 新增：

```text
KAROQ_CAN: TWAI start OK: RX=GPIO2, TX=GPIO1(unwired), listen-only, 500 kbps
KAROQ_UI: Using FINAL BRANCH HUD: speed label x=481 y=47
KAROQ_UI: V1.0.2 V0.6 BRANCH UI: CAN 0x0FD speed; ACC 0x30C probe; TEMP demo guard
KAROQ_CORE: FW v1.0.2 | UI=CAN_SNIFFER_V0.6 | SPEED=0x0FD CAN RX | ACC=0x30C probe-only | guard=TEMP
```

這些是**預期格式**，不是已取得的 V1.0.2 實車 Log。完整 IDF 編譯與 LCD 更新需要在你的 Windows 環境測試。

## 3. 下一階段：移除 Demo writer 與 Guard

先用唯讀工具找出實際程式碼：

```powershell
python D:\Karoq_HUD\Karoq_HUD_V1_0_2_V06_UI_Release\tools\audit_v06_speed_demo.py --file D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_2\main\ui\v0_6\karoq_hud.c > D:\Karoq_HUD\V06_UI_speed_demo_report.txt
```

確認哪一個 LVGL timer 在建立原 HUD 後又向速度 Label 寫入 `88`，**只停用該 Demo 速度更新**，保留原 Label 建立、字型、位置和其餘動畫／導覽。完成原始碼實測後，才能從 CMake 移除 `ui/guard/karoq_hud_speed_guard.c`，並讓 speed bridge 不再調用 Guard。

## 4. 資料夾與安全

```text
Karoq_CAN_HUD_V1_0_2/
├── main/
│   ├── core/          主程序、版本資訊
│   ├── id/
│   │   ├── can/       TWAI 接收
│   │   ├── decoders/  0x0FD / 0x30B / 0x30C
│   │   ├── signals/   未來 Lane / Sign / BAP
│   │   └── profiles/  Karoq/MQB 車型設定
│   ├── ui/
│   │   ├── v0_6/      唯一正式 V0.6 介面與資產
│   │   ├── bridge/    CAN 車速 → 原速度 Label
│   │   ├── guard/     暫存 Demo 防覆寫，待正式停用 Demo 後移除
│   │   ├── widgets/   預留
│   │   └── screens/   預留
│   └── other/         probes / diagnostics / nav / storage
├── docs/              UI SHA-256，CAN ID 表，測試規格
├── tests/             離線測試
└── history/           升級前原碼備份
```

車輛 CAN 全程 Listen Only。不要在駕駛過程操作筆電或觀看診斷資訊；靜態功能試驗在安全停車狀態進行，動態速度確認由乘客協助觀察。
