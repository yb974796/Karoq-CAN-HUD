# 10run — V1.0.5 comma.ai Openpilot MQB 整合審查

## 範圍
- 現有 V1.0.4 及 V0.6 UI 不能被替換；CAN 全程 **TWAI Listen Only**。
- `opendbc/dbc/vw_mqb.dbc` 與 `carstate.py` 來源：詳見 `OPENDDBC_SOURCE.md`，不採納可能錯誤的論壇轉述。
- 下表 10 項主機單元測試全部執行 `tests/run_10run.sh`，不等於 10 次實車／上路測試。

## 十項測試與重要發現

| RUN | 測試 | 原始問題或安全風險 | 整合處理 | 狀態 |
|---|---|---|---|---|
| 01 | ID registry count and uniqueness | 同 CAN ID 跨車型含義不同、易撞 ID | 訊號單獨存於 `id/signals`，明確 MQB 限定 | 離線 PASS |
| 02 | `0x0FD` 原始速度與 QBit | 把靜止值當作完整精度驗證；QBit 極性未證 | 速度繼續使用既有實車模組，metadata 僅比對 | 離線 PASS |
| 03 | `0x0B2` 四輪輪速 | 誤把 ESP_21 當作 openpilot 唯一車速來源 | 分別記錄 4 輪，各 0.0075km/h | 離線 PASS |
| 04 | `0x120` TSK 狀態 0–7 | V1.0.4 只看 `0x30C DISP`，無法證明 ACC Active | 原始欄位解碼，openpilot 2–5 Available、3–5 Enabled、6–7 Fault | 離線 PASS、實車待驗 |
| 05 | `0x30C` ACC setpoint/顯示 | raw=`0x3FF` 表示不可用；DISPLAY=6 不等於啟用 | 保留 raw 並分開提供設定速度與顯示碼 | 離線 PASS |
| 06 | `0x397` 車道欄位 | LED green 單 bit 不等於 Lane/Travel Active | 僅收集 warning/LED/學習狀態，**不改 V0.6 UI** | 離線 PASS、實車待驗 |
| 07 | EPS `0x09F` | EPS 狀態不等於相機偵測有車道線 | 讀 EPS_HCA_Status 但不直接映射車道綠色圖示 | 離線 PASS |
| 08 | `0x12B` 巡航按鍵 | 忽略 SET/RES 按鍵交叉訊號 | 提供 raw 解碼用於未來按鈕對照 | 離線 PASS |
| 09 | 11/29bit、RTR、DLC 不合格式 | 誤解碼 29-bit 同數值 ID 或 DLC<8 | 拒絕不合格式訊框，**SD 全量紀錄不過濾** | 離線 PASS |
| 10 | 隨機 Intel bit 解析 | 跨位元組 10-bit/16-bit 讀錯會誤顯示 | 大量 bit-window 與獨立 64-bit 參考演算法比較 | 離線 PASS |

## 完成的整合
- 新增 `main/id/signals/karoq_mqb_opendbc.c/.h`，純 C，不依賴 LVGL。
- 新增 `main/other/diagnostics/karoq_mqb_openpilot_probe.c/.h`，使用原 **TWAI RX 任務**的一份訊框副本，定期輸出已出現的 12 個 MQB 候選 ID、變化量及資料年齡。
- **不創建第二組 TWAI、不做 TX、不修改 SD logger 採樣、不改車速與 ACC 既有 decoder，不改 HUD V0.6。**
- `tools/analyze_mqb_can.py` 可離線讀取 `CAN_XXXX.csv` 全量檔或舊的 `RX ... NEW_ID` 串口日誌，列出見到/沒見到的 ID。
- 升級器原封保留 V0.6 UI 檔案位元並做 SHA-256 核對；過往速度 demo Guard 繼續運作。

## 真實日誌比對
舊的 255 unique `NEW_ID` 日誌中，12 個候選 ID 裡有 **8 個已出現**：`0x0FD,0x0B2,0x30B,0x30C,0x12B,0x09F,0x30F,0x366`。其餘 `0x120,0x122,0x397,0x126` 未在**該份**紀錄觀察到。更長的 SD 全量 Log 才能確定動態訊號、時間差與訊框出現條件。不能因為不在單次掃描裡就宣稱功能不存在。

## 新發現的 10 項後續風險／建議
1. CAN bus 確認：J533/PT/CAM/EXT 路徑可能不同，不能直接將未見的 `0x397` 判作攝影機故障。
2. ACC Active 以 `0x120` 為 openpilot 主要依據，但在 HUD Comfort CAN 上能否取到要重新錄影確認。
3. `0x30C DISP` 曾在靜止時 0/6 快速切換，禁止直接驅動綠色 ACC 圖示。
4. `0x397 LDW_Status_LED_gruen` 是儀表 LED 信息，不代表雙側 Lane 偵測或 Travel Assist 持續控制。
5. `0x126 HCA_01` 涉及 steering 相關訊息；本裝置不得重播、修改或發送。
6. `0x0FD` 的 QBit 訊號極性仍需實車驗證，不能把未驗證的 bit 值當作 valid 狀態。
7. `0x30B` 儀表速度與 `0x0FD` ESP 車速有物理意義不同，動態比較時須注意偏差。
8. SD Logger `_CHANGED.csv` 受 rolling counter/checksum 影響會放大變化量，不能直接等於「駕駛狀態改變」。
9. 速限交通標誌及 CarPlay 導航沒有從這 12 ID 建立可靠映射，後續要另外分析 BAP／VC／車機來源；不含紅綠燈。
10. 目前只有主機端 bit/整合驗證；完整 ESP-IDF 5.5.3 建置、10 分鐘 SD Logger 寫入、實際按鍵及實車多網段同步對照均待完成。
