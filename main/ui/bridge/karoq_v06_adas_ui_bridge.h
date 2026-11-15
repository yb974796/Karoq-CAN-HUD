#pragma once
#include "lvgl.h"
#include "karoq_mqb_assist_state.h"
#ifdef __cplusplus
extern "C" {
#endif
// Must be called from original V0.6 UI creation / LVGL lock AFTER existing icons exist.
// Icon image and four original image-dsc pointers must be explicit, never guessed by geometry.
void karoq_v06_adas_bind_image(lv_obj_t *assist_image,
                              const void *off_img, const void *acc_img,
                              const void *lane_img, const void *travel_img);
// Called after karoq_hud_ui_init() while LVGL lock is held.
void karoq_v06_adas_ui_attach(void);
#ifdef __cplusplus
}
#endif
