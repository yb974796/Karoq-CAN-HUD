# V1.0.2 Test Plan

1. `Upgrade_V1_0_2.ps1` 安裝獨立專案，必須顯示所有 V0.6 UI SHA256 完全一致。原 V1.0.1 不可被更動。
2. `Verify_V1_0_2.ps1` 必須通過：一次 CAN RX、一次原 UI、一次速度 bridge、CMake script-mode 相容、禁止 V0.8 Overlay。
3. `idf.py reconfigure` + `idf.py build`，不應出現 component CMake error 或 unresolved speed symbols。
4. `idf.py -p COM8 flash monitor`，需出現 `KAROQ_CORE: FW v1.0.2` 及 `KAROQ_UI: V1.0.2 V0.6 BRANCH UI`。
5. 原分支 640×172 圖示、導航、速限與車速文字外觀與 V0.6 一致；車速 0x0FD 不跳回 88，Guard 若觸發則 LOG 會顯示一次。
6. 記錄 ≥10 分鐘：RX 累計、lost、overrun、bus_err、車速 age。ACC `0x30C` 只記錄原始碼，不推定 0/6 等於 engaged。
7. `python tools/audit_v06_speed_demo.py --file main/ui/v0_6/karoq_hud.c` 建立 Demo writer 報告；此步不會修改任何車輛或程式碼。
