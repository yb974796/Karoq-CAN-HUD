#pragma once
/* Include at the very beginning of ORIGINAL main/karoq_hud.c only.
 * lvgl.h is loaded completely before macros are defined, so LVGL's function
 * prototypes are not modified. Other translation units stay unaffected. */
#include "karoq_hud_speed_guard.h"
#define lv_label_set_text(obj, text) \
    karoq_hud_speed_guard_label_set_text((obj), (text))
#define lv_label_set_text_static(obj, text) \
    karoq_hud_speed_guard_label_set_text_static((obj), (text))
#define lv_label_set_text_fmt(obj, fmt, ...) \
    karoq_hud_speed_guard_label_set_text_fmt((obj), (fmt), ##__VA_ARGS__)
