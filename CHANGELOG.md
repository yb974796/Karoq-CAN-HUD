# V1.0.6 (2026-10-31)

- 12 ID read-only opendbc decoder 繼續沿用 V1.0.5，新增 `karoq_mqb_assist_state.c/.h`，把 0x120/0x30C/0x397/0x09F/0x126 整合為安全保守的輔助狀態。
- ACC-only 狀態需要 TSK 3～5 連續三筆及 4-bit rolling counter 依序變化，還需要有效 ACC_02 設定速度和非 limiter 時距。
- 缺訊、逾時、故障、無效設定速度則回到 UNKNOWN/灰色；Lane/Travel 只作候選分析，沒有驗證前不顯示 ACTIVE。
- `karoq_mqb_assist_runtime.*` 安全地從既有 TWAI RX 的副本取得訊息，供 LVGL timer 讀取；不建立第二組 CAN 控制器。
- `karoq_v06_adas_ui_bridge.*` 在原 LVGL 執行緒每 100ms 更新**原 V0.6 圖示 image**，若無法辨識圖示則保持原始圖示不變並報告 UNBOUND。沒有新 overlay。
- 提供 V1.0.5 → V1.0.6 fail-closed 升級器，備份+SHA256，維持速度、ACC 舊解碼、SD Logger、BOOT 按鈕原碼不變。
- 13/13 狀態機單元測試與升級兩路徑測試通過。Windows ESP-IDF 全工程、實車 UI 對照待測。
