#pragma once
#include "lvgl.h"
/* 70x104 native ARGB8888 sprites, 0..9 and dash; no runtime rasterization. */
#define KAROQ_SPEED_DIGIT_W 70
#define KAROQ_SPEED_DIGIT_H 104
const lv_image_dsc_t *karoq_speed_glyph(char ascii_digit_or_dash);
