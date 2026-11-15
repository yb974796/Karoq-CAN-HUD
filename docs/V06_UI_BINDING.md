# V0.6 UI adapter contract

升級器不會猜測左上 icon 的 x/y 或覆蓋為另一款 UI；僅當原始 `karoq_hud.c` 有**單一且確切**的 `lv_obj_t*` 原輔助圖示 image，並且有 `assist_off`、`assist_acc`、`assist_lane`、`assist_travel` 四個已存在的 image 描述符時，才插入：

```c
karoq_v06_adas_bind_image(ui_assist, &assist_off, &assist_acc, &assist_lane, &assist_travel);
```

若原版是使用多個 LVGL object / canvas / 自繪矩形，不會擅自建立新圖示或硬改資產。V1.0.6 的 CAN→狀態→LVGL Timer 仍會正常運行，但不更動左側圖示，COM8 顯示 UNBOUND，需從 `main/ui/v0_6/karoq_hud.c` 的真實 UI 更新 API 以相同狀態連接。

**備註：原始碼若採不同圖示命名，仍可手動在 V0.6 UI 建立成功後調用 `karoq_v06_adas_bind_image`。** 這不是新介面。安裝程式會在 `docs/V106_LINK_STATUS.txt` 明確標示是否找到匹配。
