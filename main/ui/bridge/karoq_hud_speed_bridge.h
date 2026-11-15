#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Called exactly once, after karoq_hud_ui_init(), under the LVGL lock.
 * It DOES NOT create any visual objects or modify the original HUD layout.
 */
void karoq_hud_speed_bridge_attach(void);

/* Optional deterministic binding if karoq_hud.c later exposes the real
 * speed label pointer. Called under LVGL lock, before/after attach.
 */
void karoq_hud_speed_bridge_bind_existing(lv_obj_t *speed_label);

#ifdef __cplusplus
}
#endif
