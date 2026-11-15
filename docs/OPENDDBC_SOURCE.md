# comma.ai / opendbc 來源與可驗證範圍

取得於 2026-10-31，GitHub `commaai/opendbc` 主分支：

1. `opendbc/dbc/vw_mqb.dbc`: https://github.com/commaai/opendbc/blob/master/opendbc/dbc/vw_mqb.dbc
   - Git blob SHA `bfa397188ad2bbea3bd0e766856a73e18054b4ff`
   - 本專案選出 12 種 CAN frame、39 個唯讀欄位的 bit/length/scale。表內只含小端序無號整數 @1+，其他訊號須另外驗證。
2. `opendbc/car/volkswagen/carstate.py`:
   https://github.com/commaai/opendbc/blob/master/opendbc/car/volkswagen/carstate.py
   - Git blob SHA `e535246420e10b2c43e5d0798766d229cfec806a`
   - `update_mqb` 的 ACC: `TSK_06.TSK_Status` 2–5 available、3–5 enabled、6–7 fault。
   - `ACC_02.ACC_Wunschgeschw_02` 為 ACC 設定速度來源（前提 `pcmCruise`）。
   - `Kombi_01.KBI_angez_Geschw` 是儀表顯示速度；openpilot 車速計算主要使用 `ESP_19` 四輪輪速。
   - `LDW_02` 使用在 `Bus.cam`，對車道示意圖的解讀不能混用 `HCA_01` 與 EPS 狀態。
3. `opendbc/car/volkswagen/values.py`:
   https://github.com/commaai/opendbc/blob/master/opendbc/car/volkswagen/values.py
   - Git blob SHA `9cc5d7f70b1212e8c7eb184aff773657bddd53ff`
   - `CanBus.pt`, `CanBus.cam`, `CanBus.ext` 是對 openpilot panda/harness 的軟體 CAN 通道枚舉，**不保證**與使用者擷取 Karoq HUD 與儀表間 Comfort CAN 具備相同訊號可見性。

**限制**：DBC 是車系訊號的定義來源，不等於你的 2019 Karoq A5/雷達/VC 實車都在同一段 CAN 上輸出。資料已開啟 `opendbc` 程式碼的版本來源，並非完整車系測試證明。
