#include "karoq_v06_live_bridge.h"
#include "karoq_reference_layout.h" /* KAROQ_REFERENCE_UI_V113 */
#include "karoq_v06_live_model.h"
#include "karoq_hud.h"
#include "lvgl.h"
#include "karoq_ts_service.h"
#include "esp_timer.h"
#include <stdio.h>
#include "esp_log.h"
#include "karoq_sd_logger.h"

/* The only HUD writer. Original V0.6 provides sprite digits and icon assets. */
static lv_timer_t *s_timer;
static bool s_logged_live;
static lv_obj_t *s_vze_sign; /* VZE child on the existing native screen */
static lv_obj_t *s_vze_digits;
static lv_obj_t *s_vze_digits_stroke; /* KAROQ_VZE_VISUAL_BOLD_V113_1PX */
static uint16_t s_vze_last_value;
static lv_obj_t *s_sd_rec_dot; // native screen child, not a competing overlay writer

static karoq_assist_state_t to_native(karoq_link_assist_t state)
{
    switch (state) {
    case KAROQ_LINK_OFF: return KAROQ_ASSIST_OFF;
    case KAROQ_LINK_ACC_ONLY: return KAROQ_ASSIST_ACC_ONLY;
    case KAROQ_LINK_LANE: return KAROQ_ASSIST_LANE_ACTIVE;
    case KAROQ_LINK_TRAVEL: return KAROQ_ASSIST_TRAVEL_ACTIVE;
    default: return KAROQ_ASSIST_UNKNOWN;
    }
}

static void tick(lv_timer_t *timer)
{
    (void)timer;
    karoq_v06_live_model_t model = karoq_v06_live_model_read();
    karoq_hud_state_t hud = {
        .assist_state = to_native(model.assist),
        .speed_valid = model.speed_valid,
        .speed_kph = model.speed_kph,
        .source = model.speed_valid ? KAROQ_SOURCE_CAN_VERIFIED : KAROQ_SOURCE_NONE,
    };
    karoq_hud_ui_set_state(&hud);  /* original V0.6 API, one LVGL writer */
    karoq_ref_layout_tick((uint32_t)(esp_timer_get_time()/1000ULL));
    // Logger sets active only after successful SD mount and CSV open.
    // The dot lives inside the same LVGL screen and is never driven by BOOT taps.
    if (s_sd_rec_dot) {
        if (karoq_sd_logger_active())
            lv_obj_remove_flag(s_sd_rec_dot, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(s_sd_rec_dot, LV_OBJ_FLAG_HIDDEN);
    }
    /* VZE source intentionally unbound until CAN field is validated.
     * Hidden on unknown, stale and invalid. This is NOT vehicle speed/ACC speed.
     * All LVGL calls run on the existing 100ms single-writer timer.
     */
    if (s_vze_sign) {
        const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000LL);
        const karoq_ts_snapshot_t sign = karoq_ts_service_snapshot(now_ms);
        if (sign.valid) {
            if (s_vze_digits && sign.limit_kmh != s_vze_last_value) {
                char value[8];
                (void)snprintf(value, sizeof(value), "%u", (unsigned)sign.limit_kmh);
                lv_label_set_text(s_vze_digits, value);
                lv_obj_center(s_vze_digits);
                if (s_vze_digits_stroke) {
                    lv_label_set_text(s_vze_digits_stroke, value);
                    lv_obj_align_to(s_vze_digits_stroke, s_vze_digits, LV_ALIGN_TOP_LEFT, 1, 0);
                }
                s_vze_last_value = sign.limit_kmh;
            }
            lv_obj_remove_flag(s_vze_sign, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_vze_sign, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (model.speed_valid && !s_logged_live) {
        ESP_LOGI("KAROQ_UI", "V1.0.10 NATIVE V0.6: real CAN speed glyphs LIVE; HUD demo disabled");
        s_logged_live = true;
    }
}

void karoq_v06_live_bridge_attach(void)
{
    if (s_timer) return;
    // Called immediately after karoq_hud_ui_init(), on the LVGL task.
    // Keep all original V0.6 sources and art assets byte-identical.
    s_sd_rec_dot = lv_obj_create(lv_screen_active());
    if (s_sd_rec_dot) {
        lv_obj_set_size(s_sd_rec_dot, 7, 7);
        lv_obj_set_pos(s_sd_rec_dot, 626, 5);
        lv_obj_set_style_bg_color(s_sd_rec_dot, lv_color_hex(0x27DD55), 0);
        lv_obj_set_style_bg_opa(s_sd_rec_dot, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_sd_rec_dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_sd_rec_dot, 0, 0);
        lv_obj_set_style_shadow_width(s_sd_rec_dot, 0, 0);
        lv_obj_set_style_pad_all(s_sd_rec_dot, 0, 0);
        lv_obj_remove_flag(s_sd_rec_dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(s_sd_rec_dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(s_sd_rec_dot, LV_OBJ_FLAG_HIDDEN);
    }
    /* V0.6 left-bottom reserved area, never change original artwork.
     * White circular sign, red rim, black numerals. Initially hidden.
     * No extra timer or overlay screen: this remains a single LVGL writer.
     */
    s_vze_sign = lv_obj_create(lv_screen_active());
    if (s_vze_sign) {
        lv_obj_set_pos(s_vze_sign, 27, 112);
        lv_obj_set_size(s_vze_sign, 48, 48);
        lv_obj_set_style_radius(s_vze_sign, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_vze_sign, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(s_vze_sign, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_vze_sign, lv_color_hex(0xE12727), 0);
        lv_obj_set_style_border_width(s_vze_sign, 4, 0);
        lv_obj_set_style_shadow_width(s_vze_sign, 0, 0);
        lv_obj_set_style_pad_all(s_vze_sign, 0, 0);
        lv_obj_remove_flag(s_vze_sign, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(s_vze_sign, LV_OBJ_FLAG_CLICKABLE);
        s_vze_digits = lv_label_create(s_vze_sign);
        if (s_vze_digits) {
            lv_obj_set_style_text_color(s_vze_digits, lv_color_hex(0x111111), 0);
            lv_obj_set_style_text_font(s_vze_digits, &lv_font_montserrat_14, 0);
            lv_label_set_text(s_vze_digits, "--");
            lv_obj_center(s_vze_digits);
            /* 1px overlay makes the 14px numeric glyph look heavier. */
            s_vze_digits_stroke = lv_label_create(s_vze_sign);
            if (s_vze_digits_stroke) {
                lv_obj_set_style_text_color(s_vze_digits_stroke, lv_color_hex(0x111111), 0);
                lv_obj_set_style_text_font(s_vze_digits_stroke, &lv_font_montserrat_14, 0);
                lv_label_set_text(s_vze_digits_stroke, "--");
                lv_obj_align_to(s_vze_digits_stroke, s_vze_digits, LV_ALIGN_TOP_LEFT, 1, 0);
            }
        }
        lv_obj_add_flag(s_vze_sign, LV_OBJ_FLAG_HIDDEN);
    }
    karoq_ref_layout_attach(s_vze_sign, s_sd_rec_dot);
    s_timer = lv_timer_create(tick, 100, NULL);
    if (!s_timer) {
        ESP_LOGE("KAROQ_UI", "Failed to create native V0.6 HUD update timer");
        return;
    }
    lv_timer_ready(s_timer);
    ESP_LOGI("KAROQ_UI", "V1.0.10 V0.6 native: ACC + independent EPS Lane; SD REC dot; single writer");
}
