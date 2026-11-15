# Karoq CAN HUD V1.0.6 — Openpilot MQB → 原 V0.6 HUD 連動

**唯一指定 UI**：「分支 · 專案分析 CAN sniffer」的 Karoq CAN HUD V0.6，640×172 黑底。原有速度 Label、導航／速限、BO​OT SD Logger 全部保留。

這是 **V1.0.5 → V1.0.6 的原始碼升級包**，不是獨立 .bin。它會在你的 Windows 現有完整 V1.0.5 專案中，連接 CAN → ADAS 狀態機 → LVGL 更新。

## 1. 已完成的訊號連動

- `0x120 TSK_06 / TSK_Status`：依 comma.ai `carstate.py`，2～5 可用，3～5 啟用，6～7 故障。畫面只有在訊框新鮮、連續三筆同狀態，且 Counter 逐筆正確增加時，才考慮 ACC 啟用。
- `0x30C ACC_02`：設定速度 `12|10@1+ × 0.32 km/h`、設定時距 `37|3`；raw `0x3FF` 無效，設定速度大於 250 km/h 或時距 0 時不顯示 ACC 綠色。`DISP=0/6` 不被視為啟用依據。
- `0x397 LDW_02` + `0x09F LH_EPS_03` + `0x126 HCA_01`：擷取 Lane 線索並計算 `lane_candidate`，**不會**直接把它等同車道置中 Active。Travel Assist 尚無已確認的獨立啟用證據，因此不會亮綠色。
- **逾時退灰**：TSK 超過 650ms、ACC 超過 1200ms，狀態回到未知／灰色，絕不維持失聯前的綠燈。
- **CAN 全程 Listen Only**：沿用唯一 TWAI RX；不新增發送動作，不操控 ACC、轉向、煞車或油門。

注意：ACC 綠燈條件是基於 opendbc、op​enpilot 的保守初版判斷，尚非已完成 Karoq 實車按鍵及儀表狀態對照。**此 DIY HUD 不能當作功能啟用或道路安全的唯一依據。**

## 2. 最重要：原 V0.6 HUD 圖示資產綁定

此升級器會讀取你本機的 `main/ui/v0_6/karoq_hud.c`，並嘗試安全地辨識 V0.6 左上方 *原本* 的 LVGL assist image 物件與四個 image-dsc 資產：`assist_off`、`assist_acc`、`assist_lane`、`assist_travel`。**只有完全辨識成功才會修改 `karoq_hud.c` 加上綁定呼叫**；不更換介面、不加 overlay、不任意掃描 x/y 猜圖示物件。

若 V0.6 原始碼使用不同命名／自繪向量圖，升級器仍會連上 CAN 狀態資料與 LVGL Timer，但會回報：

```text
V0.6 icon binding: UNBOUND
KAROQ_UI: ... image=UNBOUND (requires V0.6 icon binding)
```

這種情況下**不會宣稱原左上圖示已經隨 CAN 切換**。可在 `main/ui/v0_6/karoq_hud.c` 中，LVGL image 建立後以真正的物件與素材名稱加入：

```c
#include "karoq_v06_adas_ui_bridge.h"
// 放在 karoq_hud_ui_init() 內，四個素材是現有原始素材的 LVGL image descriptor
karoq_v06_adas_bind_image(existing_assist_image,
                           &existing_off_img,
                           &existing_acc_img,
                           &existing_lane_img,
                           &existing_travel_img);
```

`existing_*` 是佔位名稱，**不能照抄直接編譯**。綁定到圖片後，V0.6 原圖示僅以 `lv_image_set_src()` 切換，不會改動尺寸或位置。若原版圖示是多個 LVGL 線條／車輛物件而非單張 image，需要改採原版 UI 已有的上色 API；不可自動猜測。請依升級後 `docs/V106_LINK_STATUS.txt` 的報告判斷。

## 3. 安裝

解壓 ZIP 至 `D:\Karoq_HUD`，在 ESP-IDF 5.5.3 PowerShell：

```powershell
cd D:\Karoq_HUD\Karoq_HUD_V1_0_6_Openpilot_ADAS_UI_Link
.\Upgrade_V1_0_6.ps1 -SourceProject 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_5' -Destination 'D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_6'

cd D:\Karoq_HUD\Karoq_CAN_HUD_V1_0_6
python D:\Karoq_HUD\Karoq_HUD_V1_0_6_Openpilot_ADAS_UI_Link\tools\verify_v106.py --project .
Get-Content .\docs\V106_LINK_STATUS.txt
idf.py reconfigure
idf.py build
idf.py -p COM8 flash monitor
```

安裝器先複製 V1.0.5 到**新資料夾**，不更動來源。`history/V1_0_5_ADAS_UI_Link` 備份被修改的原始檔，並保存 V0.6 原資產的 SHA-256 對照。若專案形態不同，工具會拒絕盲目修補。

## 4. 預期日誌與實車安全驗證

```text
KAROQ_ADAS: V1.0.6 passive ADAS link initialized; TX disabled; V0.6 HUD baseline
KAROQ_UI: V1.0.6 ADAS UI timer started; original V0.6 style unchanged, image=BOUND
KAROQ_ADAS: view=UNKNOWN TSK=0 age=... ACC_age=... LDW_green=0 ...
KAROQ_ADAS: view=ACC_ONLY TSK=3 ...
```

上面是**程式輸出格式示例，不是已取得的 Karoq 實車 Log**。尤其前一版 SD Log 未在同網段觀察到 `0x120/0x397/0x126`，因此若 COM8 長時間 `UNKNOWN`，先檢查 CAN bus 路徑，不要強行點亮圖示。

在安全停車狀態先確認沒訊號／熄火退灰；再由副駕在受控環境錄製 ACC OFF／STANDBY／ACTIVE 以及 Lane／Travel 使用場景，對照原車儀表、SD 完整 CAN CSV 與 COM8。車輛行駛中不要由駕駛調整接線、操作筆電或依賴 DIY HUD 判斷安全控制狀態。

## 5. 驗證界線

這份程式已通過 13 項新狀態機測試、ESP-IDF/LVGL 模擬標頭語法檢查、兩種 UI 綁定情況的遷移測試及上一版解碼和記錄器回歸測試。**尚未完整在 Windows ESP-IDF 5.5.3 建置、於實體 LCD 顯示或實車驗證 ACC／Lane／Travel 真實狀態。**
