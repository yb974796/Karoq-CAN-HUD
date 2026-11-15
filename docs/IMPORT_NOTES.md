# V1.0.2 UI 匯入說明

此版本**不包含使用者未提供的原始 `karoq_hud.c`、`karoq_hud_assets.c`、字型或 Waveshare BSP**。`Upgrade_V1_0_2.ps1` 從已成功實測的 V1.0.1 工程 `main/ui/legacy` 移入所有檔案至 `main/ui/v0_6`。

`docs/UI_V06_SHA256.json` 提供搬移前後所有原 UI 檔案的 SHA256 校驗。校驗證明**保留原檔內容**，並非對原分支版本來源做外部認證；請確認來源工程是你指定的 CAN sniffer V0.6 畫面。

其他 ID、CAN、ACC/速度原碼與已測試工程相同；`history/v1_0_1_backup` 保留主要修改前的檔案。`ui/guard` 暫時留用，待取得實際 Demo 速度更新原始碼後，先移除寫入者再移除 Guard。
