# MQB → HUD V0.6 顯示狀態矩陣

| CAN ID | 原始欄位（opendbc VW MQB） | 綁定用途 | 來源驗證與限制 |
|---|---|---|---|
| 0x120 TSK_06 | `TSK_Status` 24|3@1+、COUNTER 8|4@1+ | 巡航可用、啟用、故障基礎 | opendbc + openpilot 規則；本 Karoq tapped CAN 尚未觀察到此 ID |
| 0x30C ACC_02 | `ACC_Wunschgeschw_02` 12|10@1+ ×0.32、`ACC_Gesetzte_Zeitluecke` 37|3 | ACC 啟用交叉佐證 | 實車觀察過 0x3FF 特殊值；不以 DISPLAY=0/6 判斷啟用 |
| 0x397 LDW_02 | `LDW_Status_LED_gruen` 62|1、警示位元 | Lane 候選 | LED 不是自動轉向已接管的充分證據 |
| 0x09F LH_EPS_03 | `EPS_HCA_Status` 32|4 | EPS 候選 | 單個狀態碼不是 Lane Active |
| 0x126 HCA_01 | `HCA_01_Status_HCA` 32|4 | 相機控制候選 | 單個狀態碼不是 Travel Active |

正式 V0.6 UI 的 `UNKNOWN/OFF` 全灰；`ACC_ONLY` 車／ACC 綠、車道線灰；`LANE_ACTIVE/TRAVEL_ACTIVE` 暫不從未驗證的單一位元觸發。UI 綁定必須是原物件；沒有該對應物件就保留原 UI、COM8 顯示 UNBOUND，不造新圖層。

## 10run 結論

1. 缺 TSK → UNKNOWN。
2. TSK 待命 → OFF。
3. TSK3 與 ACC 連續交叉證據 → ACC_ONLY。
4. ACC 0x3FF → UNKNOWN。
5. ACC gap0 → UNKNOWN。
6. TSK timeout → UNKNOWN。
7. ACC timeout → UNKNOWN。
8. TSK fault6 → UNKNOWN。
9. LDW+EPS+HCA 同時存在 → Lane candidate，而非 Active。
10. ACC+Lane 候選 → ACC_ONLY，不假稱 Travel。
11. 擴展幀、RTR、DLC<8 → 拒絕。
12. TSK Counter 不遞增 → 不點亮。
13. ACC setpoint 不合理值 → 不點亮。

碼表與操作邏輯來源：
- https://github.com/commaai/opendbc/blob/master/opendbc/dbc/vw_mqb.dbc
- https://github.com/commaai/opendbc/blob/master/opendbc/car/volkswagen/carstate.py

來源為特定版本的 opendbc，車型/硬體、拓樸仍需本車真實 Log 驗證。
