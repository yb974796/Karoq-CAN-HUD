#pragma once
#include "lvgl.h"
#include <stdint.h>
/* Attach on the same LVGL task, after the original V0.6 screen and VZA sign exist. */
void karoq_ref_layout_attach(lv_obj_t *vza_sign, lv_obj_t *sd_dot);
/* Refresh after original V0.6 bridge tick; never start another UI writer. */
void karoq_ref_layout_tick(uint32_t now_ms);
