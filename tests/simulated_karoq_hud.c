/* Mirrors the exact first-line include automatically inserted in karoq_hud.c */
#include "karoq_hud_speed_intercept.h"
void simulate_original_hud_demo(lv_obj_t *speed, lv_obj_t *nav)
{
    lv_label_set_text(speed,"88");
    lv_label_set_text_fmt(speed,"%d",88);
    lv_label_set_text_static(speed,"88");
    lv_label_set_text_fmt(nav,"NEXT TURN %d",7);
}
