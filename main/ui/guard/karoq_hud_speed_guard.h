#pragma once
/* Safe API — original UI's demo-speed writes are suppressed only for the
 * exact speed label bound by the V0.9/V0.10 bridge. Do not call from CAN task. */
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void karoq_hud_speed_guard_bind(lv_obj_t *speed_label);
uint32_t karoq_hud_speed_guard_blocked_total(void);
void karoq_hud_speed_guard_label_set_text(lv_obj_t *obj, const char *text);
void karoq_hud_speed_guard_label_set_text_static(lv_obj_t *obj, const char *text);
void karoq_hud_speed_guard_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...);
#ifdef __cplusplus
}
#endif
