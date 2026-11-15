# V1.0.2 整合矩陣

| 來源 | 現在的專案功能 | 狀態 |
|---|---|---|
| 分支 Karoq CAN HUD **V0.6** | `ui/v0_6/` 原版 640×172 HUD、圖示與字型 | 自使用者本機 V1.0.1 專案移入，SHA256 檔案位元保證不變；畫面版本來源由使用者指定 |
| V0.6 / V0.7 sniffer | `id/can/` TWAI 500 kbps Listen Only，1024 RX queue，2048 槽 ID 表 | CAN 實車 Log > 100 萬框，0 lost / overrun / bus error（在該測試時段） |
| V0.7 speed | `id/decoders/` ESP_21 `0x0FD`；`0x30B` 備援 | 0x0FD 已實車，0x30B 備援尚未對照 |
| V0.8 ACC | `id/decoders/` ACC_02 `0x30C` 原始狀態 | 已有實車原始日誌，DISP=0/6 的語意仍未定 |
| V0.9 / V0.10 | `ui/bridge/` + `ui/guard/` | 原速度 Label x=481 y=47 已找到，Guard 實車攔截成功；尚未從源頭關閉 Demo |
| **V1.0.2** | `core/id/ui/other` 分目錄、固定 UI V0.6、升級／驗證／版本標記 | 本機模擬遷移及 host 測試通過；實際 IDF 編譯及 LCD 待測 |

無法僅靠 Log 斷言 ACC 狀態語意、ACC 設定速度、Lane / Travel Assist 或 HUD 畫面有已驗證的動態圖示更新。
