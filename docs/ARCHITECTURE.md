# V1.0.2 架構

```text
main/core/          ESP32-S3 LCD/LVGL 初始化、版本資訊、CAN RX 啟動
main/id/can/        TWAI 500 kbps, GPIO2 RX, GPIO1 TX 未實際接線, Listen Only
main/id/decoders/   ESP_21 0x0FD 主要車速、Kombi_01 0x30B 備援、ACC_02 0x30C 原始訊號
main/id/signals/    未來 Lane、Travel、速限標誌、導航 BAP（未定義時不能宣稱已解碼）
main/id/profiles/   車型設定/訊號位元配置預留
main/ui/v0_6/       分支定稿的真實 V0.6 LVGL UI 和其素材（原樣匯入）
main/ui/bridge/     原速度 Label 的唯一 CAN 車速橋接任務（LVGL Timer 50ms）
main/ui/guard/      過渡性的舊 HUD Demo 寫入攔截（待停用 Demo 源碼後移除）
main/ui/assets/     後續資產預留（V0.6 原資產不移出 ui/v0_6）
main/other/         探針、診斷、導航及儲存功能預留
```

IDF 5.5.3 元件主 CMake 不得使用 `CONFIGURE_DEPENDS`，因其依賴分析會在 CMake Script Mode 執行。新增 .c 到 globbed folders 後需 `idf.py reconfigure`。

CAN 接收工作絕不呼叫 LVGL；解碼透過 thread-safe snapshot；LVGL 更新僅在 UI 所屬 Timer 與 mutex 保護下。Guard 只是已驗證的暫存安全帶，正式單一 writer 架構仍待停用 UI 的原始 Demo 定時寫入後實作。
