# Karoq CAN HUD V1.0.7 — V1.0.2–V1.0.6 統合測試版

**唯一 UI**：使用者指定「分支 · 專案分析 CAN sniffer」**Karoq CAN HUD V0.6**，640×172，原有 HUD 的左上輔助、左下限速標誌、中央導航、右側大型車速維持不變。**本 ZIP 不包含使用者尚未提供的完整私有 V0.6 `karoq_hud.c` 與素材；請從目前已能開機的專案匯入，沒有原檔便拒絕虛構新介面。**

這是 **可一鍵安裝為新 ESP-IDF 專案的原始碼合併包**，非已編譯可直接燒錄的 `.bin`。包含歷版 V1.0.3～V1.0.6 所需的離線升級模組，不需要個別下載之前四個 ZIP。

## 1. 單次升級（V1.0.2–V1.0.6 任一版都可以）

下載 ZIP 並解壓至 `D:\Karoq_HUD`，使用 ESP-IDF 5.5.3 PowerShell：

```powershell
cd D:\Karoq_HUD\Karoq_HUD_V1_0_7_Consolidated_10RUN
.\Upgrade_V1_0_7.ps1 -SourceProject 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_6' -Destination 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_7'
```

若你目前最後成功的是 V1.0.2，將 `-SourceProject` 改成 `D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_2`。升級程式會按需要自動執行 V1.0.3/1.0.4/1.0.5/1.0.6 中間遷移，最後合併至 V1.0.7。**原有專案不會被覆蓋**，目標存在時也會拒絕安裝。保留已導入的 LCD 驅動、元件依賴、實際 UI、SDK 配置。

如有 UI 名稱不符，`docs/V106_LINK_STATUS.txt` 會明確記錄 `UNBOUND`：CAN 後端正常，但左上圖示不會跟著切換，不能把它誤認為已直接連動。此時須依實際 `main/ui/v0_6/karoq_hud.c` 物件及素材名稱進行定向綁定，而不是新增覆蓋介面。

## 2. 直接編譯與燒錄

```powershell
cd D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_7
python verify_v107.py --project .
Get-Content .\docs\V107_STATUS.txt
idf.py reconfigure
idf.py build
idf.py -p COM8 flash monitor
```

請等 `idf.py build` 完整成功才 flash。COM8 會顯示原 CAN RX 速率/車速、MQB ID 偵測、SD Logger 統計與 ADAS 狀態。若 V0.6 圖示無法綁定，只顯示 `UNBOUND`，不可視為 HUD 圖示已成功更新。

## 3. 已整合

- **core**：一個 `app_main()`，一次 TWAI 啟動，一次 SD Writer/Button 初始化，一次 MQB Probe/ADAS runtime 初始化，一次 V0.6 UI 與 speed/ADAS bridge。
- **id/can**：500kbps / LISTEN_ONLY / GPIO2 RX，GPIO1 TX 軟體配置但不接線；任何模組均不呼叫 `twai_transmit()`。
- **id/decoders**：ESP_21 `0x0FD` 真車速、Kombi_01 `0x30B` 備援、ACC_02 `0x30C` 原始碼。
- **id/signals**：comma.ai opendbc MQB DBC 約 12 個重要 ID 的位元解碼與 ADAS snapshot；`0x120` TSK_06 的計數器須與狀態連續三筆，斷訊重新開始計數。
- **ui/v0_6**：唯一正式 UI，不加入任何 V0.8 額外畫面；保留暫時 Guard 避免既有 demo 速度 `88` 覆寫真實速度。
- **other/storage**：SDMMC 1bit `CLK=41 CMD=39 D0=40`，BOOT GPIO0 短按一次開始再短按停止，輸出 ALL/NEW/CHANGED/META；1024 筆緩衝與 overflow 計數。
- **other/diagnostics**：每 5 秒 MQB ID 出現與資料變更，沒看到 `0x120/0x397/0x126` 不等於沒有 ADAS；可能在另一 CAN bus。

## 4. 用 SD 卡測試

停車時 FAT32 microSD 插入，短按 BOOT → COM8 `REC START`；記錄車輛狀態變化；再短按 BOOT → 等待 `STOP` 和 `queue_dropped=0`、`media_error=0`；確認 4 檔案。車速行進比對由副駕操作，勿在行駛中插拔 CAN／SD 線。**TWAI ESP32-S3 為 classic CAN 監聽，非 CAN FD 控制器**。

## 5. 驗證範圍及限制

發行包的 `tests/ten_run_unified.py` 對 V1.0.4 logger、V1.0.5 DBC、V1.0.6 ADAS、V1.0.7 counter fix 執行 10 輪離線回歸。**並非 Windows ESP-IDF 完整編譯／實車 SD 壓力測試**。在你自己的專案中仍需 `idf.py build`、實際 LCD 及 SD 連續記錄驗證。

不會把未實車對照的 Lane LED 綠 bit 自動視為車道置中，也不會僅因 ACC_02 DISP=6 判斷 ACC ACTIVE。`TSK_06` 判定參考 openpilot `CarState`，但仍要確認 Karoq 車身實際所接的 CAN 網段；DIY HUD 不作為安全功能的唯一依據。圖示可能會持續灰色：若 `0x120` 在 Comfort CAN 缺席即是合理的 fail-closed 行為。

參照原始來源：
- https://github.com/commaai/opendbc/blob/master/opendbc/dbc/vw_mqb.dbc
- https://github.com/commaai/opendbc/blob/master/opendbc/car/volkswagen/carstate.py
