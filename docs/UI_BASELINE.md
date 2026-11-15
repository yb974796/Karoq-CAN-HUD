# UI BASLINE — 唯一允許的 V0.6 UI

使用者已明確指定「分支 · 專案分析 CAN sniffer / Karoq CAN HUD V0.6」做唯一 UI 基準。這是原始的 `karoq_hud.c` + `karoq_hud_assets.c` 與依賴字型素材的完整檔案集。

輸出大小 640 × 172，黑底；左上 Lane / ACC / Travel Assist，左下道路限速標誌，中央導航，右側大型原字型白色時速。保留相對位置、大小和各圖示；不要加上 V0.8 新面板或覆蓋圖層，不顯示檔位。速度原 LVGL Label 實測找到 x=481, y=47。

V1.0.2 不包含未上傳的 `karoq_hud.c` 原檔。安裝器從使用者已驗證的 V1.0.1 內匯入並用 SHA-256 驗證其位元內容。指紋在已安裝專案 `docs/UI_V06_SHA256.json`。

**後續變動 UI 功能時，需要修改實際檔案並更新 SHA-256 基準，不能假稱完全未改；當前 V1.0.2 不觸動。**
