/* Karoq CAN HUD V0.6: 640x172, solid black, VAG icon half-size,
 * speed numerals ~2x previous font glyph height, native 70x104 sprites.
 * No navigation / traffic sign widgets; no ACC set speed or overlays.
 * All LVGL calls on the LVGL task (or with appropriate LVGL lock).
 */
#include "karoq_hud.h"
#include "karoq_hud_assets.h"
#include "karoq_hud_speed_digits.h"
#include "../layout/karoq_speed_geometry.h" /* V1145 sole coordinate writer */
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

#if LV_USE_PERF_MONITOR || LV_USE_MEM_MONITOR
#error "HUD V0.6 requires LVGL FPS/CPU/MEM monitors disabled"
#endif

#define HUD_BG            0x000000
#define HUD_WHITE         0xFFFFFF
#define HUD_SECONDARY     0xA3ADB8
#define HUD_DEMO_PERIOD   2500
#define HUD_DIGITS          3

static lv_obj_t *s_assist;
static lv_obj_t *s_speed_digits[HUD_DIGITS];
static lv_obj_t *s_status;
static lv_timer_t *s_demo_timer;
static uint32_t s_demo_ticks;
static karoq_assist_state_t s_last_state = (karoq_assist_state_t)-1;
static char s_last_speed[4] = "";
static uint32_t s_last_scale;

static const lv_image_dsc_t *image_for_assist(karoq_assist_state_t state)
{
    switch (state) {
        case KAROQ_ASSIST_LANE_ACTIVE: return &hud_img_assist_lane;
        case KAROQ_ASSIST_TRAVEL_ACTIVE: return &hud_img_assist_travel;
        case KAROQ_ASSIST_ACC_ONLY: return &hud_img_assist_acc;
        case KAROQ_ASSIST_OFF:
        case KAROQ_ASSIST_UNKNOWN:
        default: return &hud_img_assist_off;
    }
}

static lv_obj_t *create_text(lv_obj_t *root, int x, int y, int width,
                             const char *initial, const lv_font_t *font,
                             uint32_t color, lv_text_align_t align)
{
    lv_obj_t *label = lv_label_create(root);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, align, 0);
    lv_label_set_text(label, initial);
    return label;
}

static void update_text(lv_obj_t *label, const char *value)
{
    if (label && value) {
        const char *old = lv_label_get_text(label);
        if (!old || strcmp(old, value) != 0) lv_label_set_text(label, value);
    }
}

/* Speed 0..299, one/two/three digits and the unavailable "--" marker.
 * The native writer alone owns the V1.x adaptive geometry.  When a new
 * speed shares one or more digits with the previous speed, keep those image
 * sources untouched to avoid redundant LVGL invalidations.
 */
static void update_speed(const char *text)
{
    if (!text || !strcmp(text, s_last_speed)) return;
    const size_t n = strlen(text);
    if (n == 0 || n > HUD_DIGITS) return;
    const size_t previous_n = strlen(s_last_speed);
    const karoq_speed_geometry_t g=karoq_speed_geometry((unsigned)n);
    /* Apply scale/pivot at the same instant as the glyph change; the later
     * reference-layout timer must NEVER reposition these shared objects. */
    if(s_last_scale!=g.scale) {
        for(size_t i=0;i<HUD_DIGITS;++i)
            if(s_speed_digits[i]) lv_image_set_scale(s_speed_digits[i],g.scale);
        s_last_scale=g.scale;
    }
    for(size_t i=0;i<HUD_DIGITS;++i) {
        lv_obj_t *img=s_speed_digits[i];
        if(!img) continue;
        if(i<n) {
            const lv_image_dsc_t *glyph=karoq_speed_glyph(text[i]);
            if(!glyph) return;
            /* The glyph source is already correct when this character is
             * unchanged in the same slot.  Do not invalidate the LCD. */
            if(i>=previous_n || s_last_speed[i]!=text[i])
                lv_image_set_src(img,glyph);
            const int x=g.start_x+(int)i*g.scaled_width;
            if(lv_obj_get_x(img)!=x || lv_obj_get_y(img)!=g.y)
                lv_obj_set_pos(img,x,g.y);
            if(lv_obj_has_flag(img,LV_OBJ_FLAG_HIDDEN))
                lv_obj_remove_flag(img,LV_OBJ_FLAG_HIDDEN);
        } else if(!lv_obj_has_flag(img,LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(img,LV_OBJ_FLAG_HIDDEN);
        }
    }
    memcpy(s_last_speed, text, n + 1);
}

void karoq_hud_ui_set_state(const karoq_hud_state_t *d)
{
    if (!d || !s_assist || !s_speed_digits[0] || !s_status) return;
    if (s_demo_timer && d->source != KAROQ_SOURCE_DEMO) {
        lv_timer_pause(s_demo_timer);
    }
    if (s_last_state != d->assist_state) {
        lv_image_set_src(s_assist, image_for_assist(d->assist_state));
        s_last_state = d->assist_state;
    }

    /* Account for full uint16_t in the format check; 0..299 is enforced below. */
    char speed[6];
    if (d->speed_valid && d->speed_kph <= 299) {
        snprintf(speed, sizeof(speed), "%u", (unsigned)d->speed_kph);
    } else {
        strcpy(speed, "--");
    }
    update_speed(speed);

    switch (d->source) {
        case KAROQ_SOURCE_DEMO:
            update_text(s_status, "DEMO / CAN OFF");
            break;
        case KAROQ_SOURCE_CAN_VERIFIED:
            update_text(s_status, "CAN VERIFIED");
            break;
        default:
            update_text(s_status, "NO LIVE DATA");
            break;
    }
}

static void demo_tick(lv_timer_t *timer)
{
    (void)timer;
    static const karoq_assist_state_t states[] = {
        KAROQ_ASSIST_OFF,
        KAROQ_ASSIST_LANE_ACTIVE,
        KAROQ_ASSIST_TRAVEL_ACTIVE,
        KAROQ_ASSIST_ACC_ONLY,
        KAROQ_ASSIST_UNKNOWN
    };
    karoq_hud_state_t data = {0};
    data.source = KAROQ_SOURCE_DEMO;
    data.assist_state = states[(s_demo_ticks++) % 5];
    data.speed_valid = true;
    data.speed_kph = 88;
    karoq_hud_ui_set_state(&data);
}

void karoq_hud_ui_init(void)
{
    if (s_demo_timer) {
        lv_timer_delete(s_demo_timer);
        s_demo_timer = NULL;
    }
    s_demo_ticks = 0;
    s_last_state = (karoq_assist_state_t)-1;
    s_last_speed[0] = '\0';
    s_last_scale=0;
    for (unsigned i=0; i<HUD_DIGITS; ++i) s_speed_digits[i] = NULL;

    lv_obj_t *root = lv_screen_active();
    lv_obj_clean(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(HUD_BG), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    /* Native 64x31 VAG-reference bitmap, formerly 128x62. */
    s_assist = lv_image_create(root);
    lv_image_set_src(s_assist, &hud_img_assist_off);
    lv_obj_set_pos(s_assist, 22, 68);

    for (unsigned i=0; i<HUD_DIGITS; ++i) {
        s_speed_digits[i] = lv_image_create(root);
        lv_image_set_pivot(s_speed_digits[i],0,0);
        lv_obj_add_flag(s_speed_digits[i], LV_OBJ_FLAG_HIDDEN);
    }

    (void)create_text(root, 492, 122, 97, "km/h",
                      &lv_font_montserrat_14, HUD_SECONDARY,
                      LV_TEXT_ALIGN_CENTER);
    s_status = create_text(root, 474, 151, 158, "NO LIVE DATA",
                           &lv_font_montserrat_14, HUD_SECONDARY,
                           LV_TEXT_ALIGN_RIGHT);

#ifndef KAROQ_HUD_DEMO_AUTORUN
#define KAROQ_HUD_DEMO_AUTORUN 1
#endif
#if KAROQ_HUD_DEMO_AUTORUN
    s_demo_timer = lv_timer_create(demo_tick, HUD_DEMO_PERIOD, NULL);
    demo_tick(NULL);
#else
    karoq_hud_state_t unknown = {.source = KAROQ_SOURCE_NONE,
                                 .assist_state = KAROQ_ASSIST_UNKNOWN};
    karoq_hud_ui_set_state(&unknown);
#endif
}
