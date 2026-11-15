/* V1.0.13 custom UI profile from user-selected enlarged-icons 640x172 image.
 * Original six V0.6 source files are unchanged. Reposition their live children;
 * add only VZA skin, NAV and ACC SET widgets. No CAN/vehicle writes.
 */
#include "karoq_reference_layout.h"
#include "karoq_oem_nav_decoder.h" /* KAROQ_OEM_NAV_V114 */
#include "karoq_reference_assets.h"
#include "karoq_acc_green_palette.h" /* KAROQ_ACC3_V1141 */
#include "karoq_large_sign_assets.h" /* KAROQ_LARGE_VZA_V1141 */
#include "karoq_nav_clean_assets.h" /* V1144 NEW PURE VECTOR SOURCE */
#include "karoq_acc_dual_palette.h" /* V1144 GRAY/GREEN SEPARATE */
#include "karoq_hud_speed_digits.h"
#include "karoq_speed_geometry.h" /* V1145 shared speed placement */
#include "karoq_nav_service.h"
#include "karoq_ts_service.h"
#include "karoq_ui_acc_set.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define NAV_MAX_CHARS 7
#define SET_MAX_CHARS 3
/* Speed scaling/placement is owned by V0.6 native writer.
 * See karoq_speed_geometry.h: common geometry for 1/2/3 digits. */
#define ASSIST_X 20
#define ASSIST_Y 101
#define SIGN_X 31
#define SIGN_Y 8
#define UNIT_X 556
#define UNIT_Y 104
#define ASSIST_SCALE 460U   /* 180% of original 64x31 pictograms */
/* Sign and assist swapped; navigation distance remains separate. */
#define NAV_DISTANCE_X 310   /* arrow occupies x=182..301 */
#define NAV_DISTANCE_MAX_WIDTH 120 /* x310..430; speed 3-digit begins x440 */

static lv_obj_t *s_original_assist;
static lv_obj_t *s_original_speed[3];
static lv_obj_t *s_original_unit;
static lv_obj_t *s_original_status;
static lv_obj_t *s_sign;
static lv_obj_t *s_sign_image;
static lv_obj_t *s_nav_icon;
static lv_obj_t *s_nav_glyph[NAV_MAX_CHARS];
static lv_obj_t *s_cruise_icon;
static lv_obj_t *s_acc_active_dot;
static lv_obj_t *s_set_label;
static lv_obj_t *s_set_digits[SET_MAX_CHARS];
static lv_obj_t *s_set_unit;
static bool s_ready;
static uint16_t s_last_limit=UINT16_MAX;
static uint32_t s_last_distance=UINT32_MAX;
static karoq_nav_maneuver_t s_last_maneuver=KAROQ_NAV_NONE;
static uint16_t s_last_set=UINT16_MAX;
static karoq_acc_ui_mode_t s_last_acc_mode=KAROQ_ACC_UI_OFF;
static bool s_acc_ui_drawn;
static uint32_t s_last_nav_scale;
static const lv_image_dsc_t *s_nav_last_glyph[NAV_MAX_CHARS];
static bool s_speed_mismatch_active;
/* LVGL task only: incremental redraw statistics, not CAN diagnostic counts. */
static uint32_t s_ui_diag_ms;
static uint32_t s_acc_ui_changes;
static uint32_t s_nav_ui_changes;
static uint32_t s_speed_pos_changes;
/* V1.0.14.5 R3: make a short validity gap invisible to the driver.
 * Unlike CAN decode, this is a strictly UI-side hold and never emits CAN. */
#define ACC_FRESH_OFF_CONFIRM_MS 150U
#define ACC_STALE_HOLD_MS 350U
static bool s_acc_bad_pending;
static uint32_t s_acc_bad_start_ms;
static uint32_t s_acc_mode_changes;
static uint32_t s_acc_set_changes;
static uint32_t s_acc_icon_src_writes;
static uint32_t s_acc_hold_events;
static uint32_t s_acc_hold_ticks;
static uint32_t s_acc_off_confirmations;
static uint32_t s_acc_stale_ticks;
static uint32_t s_acc_last_fresh_age_ms;


static lv_obj_t *label(lv_obj_t *root,int x,int y,int w,
                       const char *txt,uint32_t color)
{
    lv_obj_t *o=lv_label_create(root);
    if(!o) return NULL;
    lv_obj_set_pos(o,x,y);
    lv_obj_set_width(o,w);
    lv_label_set_long_mode(o,LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_style_text_font(o,&lv_font_montserrat_14,0);
    lv_obj_set_style_text_color(o,lv_color_hex(color),0);
    lv_label_set_text(o,txt);
    return o;
}
static lv_obj_t *picture(lv_obj_t *parent, const lv_image_dsc_t *src,int x,int y)
{
    lv_obj_t *o=lv_image_create(parent);
    if(o){lv_image_set_src(o,src);lv_obj_set_pos(o,x,y);}
    return o;
}
static const lv_image_dsc_t *sign_image(uint16_t value)
{
    static const lv_image_dsc_t *const s[] = {
        &karoq_large_sign_5,
        &karoq_large_sign_10,
        &karoq_large_sign_15,
        &karoq_large_sign_20,
        &karoq_large_sign_25,
        &karoq_large_sign_30,
        &karoq_large_sign_35,
        &karoq_large_sign_40,
        &karoq_large_sign_45,
        &karoq_large_sign_50,
        &karoq_large_sign_55,
        &karoq_large_sign_60,
        &karoq_large_sign_65,
        &karoq_large_sign_70,
        &karoq_large_sign_75,
        &karoq_large_sign_80,
        &karoq_large_sign_85,
        &karoq_large_sign_90,
        &karoq_large_sign_95,
        &karoq_large_sign_100,
        &karoq_large_sign_105,
        &karoq_large_sign_110,
        &karoq_large_sign_115,
        &karoq_large_sign_120,
        &karoq_large_sign_125,
        &karoq_large_sign_130,
        &karoq_large_sign_135,
        &karoq_large_sign_140,
        &karoq_large_sign_145,
        &karoq_large_sign_150,
        &karoq_large_sign_155,
        &karoq_large_sign_160,
    };
    if(value<5U || value>160U || value%5U) return NULL;
    return s[(value/5U)-1U];
}
/* Do not ask LVGL to invalidate a widget that already has the right flag. */
static void visible(lv_obj_t *obj,bool show)
{
    if (!obj) return;
    const bool hidden=lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN);
    if (show && hidden) lv_obj_remove_flag(obj,LV_OBJ_FLAG_HIDDEN);
    else if (!show && !hidden) lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
}
static const lv_image_dsc_t *nav_glyph(char c)
{
    static const lv_image_dsc_t *const digits[] = {
        &karoq_ref_nav_digit_0,
        &karoq_ref_nav_digit_1,
        &karoq_ref_nav_digit_2,
        &karoq_ref_nav_digit_3,
        &karoq_ref_nav_digit_4,
        &karoq_ref_nav_digit_5,
        &karoq_ref_nav_digit_6,
        &karoq_ref_nav_digit_7,
        &karoq_ref_nav_digit_8,
        &karoq_ref_nav_digit_9,
    };
    if(c>='0' && c<='9') return digits[c-'0'];
    if(c=='m') return &karoq_ref_nav_m;
    if(c=='k') return &karoq_ref_nav_k;
    if(c=='.') return &karoq_ref_nav_dot;
    return NULL;
}
static const lv_image_dsc_t *set_glyph(char c,bool green)
{
    static const lv_image_dsc_t *const gray[] = {
        &karoq_acc_new_gray_digit_0, &karoq_acc_new_gray_digit_1,
        &karoq_acc_new_gray_digit_2, &karoq_acc_new_gray_digit_3,
        &karoq_acc_new_gray_digit_4, &karoq_acc_new_gray_digit_5,
        &karoq_acc_new_gray_digit_6, &karoq_acc_new_gray_digit_7,
        &karoq_acc_new_gray_digit_8, &karoq_acc_new_gray_digit_9
    };
    static const lv_image_dsc_t *const bright[] = {
        &karoq_acc_new_green_digit_0, &karoq_acc_new_green_digit_1,
        &karoq_acc_new_green_digit_2, &karoq_acc_new_green_digit_3,
        &karoq_acc_new_green_digit_4, &karoq_acc_new_green_digit_5,
        &karoq_acc_new_green_digit_6, &karoq_acc_new_green_digit_7,
        &karoq_acc_new_green_digit_8, &karoq_acc_new_green_digit_9
    };
    if(c>='0' && c<='9') return green?bright[c-'0']:gray[c-'0'];
    return NULL;
}
/* Only known maneuver values are rendered. A generic U-turn is ambiguous,
 * because the original enum loses the direction. Do not guess vehicle turns. */
static const lv_image_dsc_t *nav_arrow(karoq_nav_maneuver_t type)
{
    switch(type) {
        case KAROQ_NAV_STRAIGHT: return &karoq_nav_clean_straight;
        case KAROQ_NAV_LEFT: return &karoq_nav_clean_left;
        case KAROQ_NAV_RIGHT: return &karoq_nav_clean_right;
        case KAROQ_NAV_SLIGHT_LEFT: return &karoq_nav_clean_slight_left;
        case KAROQ_NAV_SLIGHT_RIGHT: return &karoq_nav_clean_slight_right;
        case KAROQ_NAV_SHARP_LEFT: return &karoq_nav_clean_sharp_left;
        case KAROQ_NAV_SHARP_RIGHT: return &karoq_nav_clean_sharp_right;
        case KAROQ_NAV_EXIT_LEFT: return &karoq_nav_clean_exit_left;
        case KAROQ_NAV_EXIT_RIGHT: return &karoq_nav_clean_exit_right;
        case KAROQ_NAV_ROUNDABOUT: return &karoq_nav_clean_roundabout;
        case KAROQ_NAV_UTURN_LEFT: return &karoq_nav_clean_uturn_left;
        case KAROQ_NAV_UTURN_RIGHT: return &karoq_nav_clean_uturn_right;
        default: return NULL;
    }
}
void karoq_ref_layout_attach(lv_obj_t *vza_sign, lv_obj_t *sd_dot)
{
    if(s_ready) return;
    lv_obj_t *root=lv_screen_active();
    if(!root || !vza_sign || lv_obj_get_child_count(root)<6U) {
        ESP_LOGE("KAROQ_REF_UI","V0.6 screen/sign not ready; style not attached");
        return;
    }
    /* First six children created by the original V0.6 native init().
     * Never delete them: the original native update task still owns pointers.
     */
    s_original_assist=lv_obj_get_child(root,0);
    for(int i=0;i<3;++i) s_original_speed[i]=lv_obj_get_child(root,i+1);
    s_original_unit=lv_obj_get_child(root,4);
    s_original_status=lv_obj_get_child(root,5);
    if(!s_original_assist || !s_original_speed[0] ||
       !s_original_speed[1] || !s_original_speed[2] ||
       !s_original_unit || !s_original_status) return;

    /* Enlarged left assist icon: bitmap scales, state switches remain native. */
    lv_image_set_pivot(s_original_assist,0,0);
    lv_image_set_scale(s_original_assist,ASSIST_SCALE);
    lv_obj_set_pos(s_original_assist,ASSIST_X,ASSIST_Y);

    /* Speed pivot/scale handled exclusively in native karoq_hud.c. */
    lv_obj_set_pos(s_original_unit,UNIT_X,UNIT_Y);
    lv_obj_set_width(s_original_unit,76);
    lv_obj_add_flag(s_original_status,LV_OBJ_FLAG_HIDDEN);

    /* Keep the original SD logger's true/false status; only move its green dot. */
    if(sd_dot) lv_obj_set_pos(sd_dot,625,13);

    /* The old 48px VZA sign is kept alive and continues to be hidden by the
     * validated VZA bridge on invalid/stale CAN signals. Only its skin changes.
     */
    s_sign=vza_sign;
    const uint32_t old_children=lv_obj_get_child_count(s_sign);
    for(uint32_t i=0;i<old_children;++i) {
        lv_obj_t *old=lv_obj_get_child(s_sign,(int32_t)i);
        if(old) lv_obj_add_flag(old,LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_pos(s_sign,SIGN_X,SIGN_Y);
    lv_obj_set_size(s_sign,90,73);
    lv_obj_set_style_radius(s_sign,0,0);
    lv_obj_set_style_bg_color(s_sign,lv_color_hex(0),0);
    lv_obj_set_style_bg_opa(s_sign,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(s_sign,0,0);
    s_sign_image=picture(s_sign,&karoq_large_sign_50,0,0);

    s_nav_icon=picture(root,&karoq_nav_clean_right,182,39);
    if(s_nav_icon) lv_obj_add_flag(s_nav_icon,LV_OBJ_FLAG_HIDDEN);
    for(int i=0;i<NAV_MAX_CHARS;++i) {
        s_nav_glyph[i]=picture(root,&karoq_ref_nav_digit_0,NAV_DISTANCE_X+i*18,89);
        if(s_nav_glyph[i]) lv_obj_add_flag(s_nav_glyph[i],LV_OBJ_FLAG_HIDDEN);
    }
    s_cruise_icon=picture(root,&karoq_acc_new_gray_cruise,457,121);
    s_acc_active_dot=lv_obj_create(root);
    if(s_acc_active_dot){
        lv_obj_set_pos(s_acc_active_dot,449,134);
        lv_obj_set_size(s_acc_active_dot,7,7);
        lv_obj_set_style_radius(s_acc_active_dot,LV_RADIUS_CIRCLE,0);
        lv_obj_set_style_bg_color(s_acc_active_dot,lv_color_hex(0x20EF15),0);
        lv_obj_set_style_bg_opa(s_acc_active_dot,LV_OPA_COVER,0);
        lv_obj_set_style_border_width(s_acc_active_dot,0,0);
        lv_obj_add_flag(s_acc_active_dot,LV_OBJ_FLAG_HIDDEN);
    }
    s_set_label=label(root,499,132,36,"SET",0xA6AEB8);
    for(int i=0;i<SET_MAX_CHARS;++i) {
        s_set_digits[i]=picture(root,&karoq_acc_new_gray_digit_0,530+i*23,121);
        if(s_set_digits[i]) lv_obj_add_flag(s_set_digits[i],LV_OBJ_FLAG_HIDDEN);
    }
    s_set_unit=label(root,599,132,40,"km/h",0xA6AEB8);
    /* OFF always shows the gray cruise icon; SET remains hidden. */
    if(s_set_label) lv_obj_add_flag(s_set_label,LV_OBJ_FLAG_HIDDEN);
    if(s_set_unit) lv_obj_add_flag(s_set_unit,LV_OBJ_FLAG_HIDDEN);
    s_ready=true;
    ESP_LOGI("KAROQ_REF_UI","V1.0.14.6 R1 20RUN: selective speed/nav glyph refresh, VZA visibility, ACC palette; RX only");
}

/* Read-only integrity check: never touch native speed image objects.
 * A mismatch means a third writer or a stale placement was detected.
 * The native update_speed() is the ONLY writer of speed positions and scales. */
static void observe_speed_positions(void)
{
    unsigned count=0;
    for(int i=0;i<3;++i)
        if(!lv_obj_has_flag(s_original_speed[i],LV_OBJ_FLAG_HIDDEN)) ++count;
    if(count==0U) {
        s_speed_mismatch_active=false;
        return;
    }
    const karoq_speed_geometry_t g=karoq_speed_geometry(count);
    int x=g.start_x;
    bool mismatch=false;
    for(int i=0;i<3;++i) {
        if(lv_obj_has_flag(s_original_speed[i],LV_OBJ_FLAG_HIDDEN)) continue;
        if(lv_obj_get_x(s_original_speed[i])!=x ||
           lv_obj_get_y(s_original_speed[i])!=g.y) mismatch=true;
        x+=g.scaled_width;
    }
    /* An uncorrected mismatch should count as ONE event, not as one event
     * per 100-ms tick.  This observer must never move speed objects. */
    if(mismatch && !s_speed_mismatch_active) ++s_speed_pos_changes;
    s_speed_mismatch_active=mismatch;
}

static void refresh_vza(uint32_t now_ms)
{
    const karoq_ts_snapshot_t ts=karoq_ts_service_snapshot(now_ms);
    const lv_image_dsc_t *im=(ts.valid && s_sign_image) ?
        sign_image(ts.limit_kmh) : NULL;
    if(!im) {
        /* This same validated service drives VZA validity.  Hiding an
         * unsupported value must not leave a stale sign on the screen. */
        visible(s_sign,false);
        s_last_limit=UINT16_MAX;
        return;
    }
    if(ts.limit_kmh!=s_last_limit) {
        lv_image_set_src(s_sign_image,im);
        s_last_limit=ts.limit_kmh;
    }
    /* Re-show after a previously hidden/unsupported sign, even when the
     * numeric limit matches the last visible value. */
    visible(s_sign,true);
}

/* Draw navigation only for a changed maneuver/distance; otherwise no LVGL IO.
 * Fixes overlap: arrow is 120px at x=182..301; distance begins at 310.
 * Width limited to <=120px to keep clear of adaptive 3-digit vehicle speed. */
static void refresh_navigation(uint32_t now_ms)
{
    const karoq_nav_snapshot_t nav=karoq_nav_service_snapshot(now_ms);
    const lv_image_dsc_t *arrow=nav.valid ? nav_arrow(nav.maneuver) : NULL;
    if (!arrow) {
        visible(s_nav_icon,false);
        for(int i=0;i<NAV_MAX_CHARS;++i) visible(s_nav_glyph[i],false);
        s_last_maneuver=KAROQ_NAV_NONE;
        s_last_distance=UINT32_MAX;
        return;
    }
    if(s_nav_icon) {
        if(nav.maneuver!=s_last_maneuver) {
            lv_image_set_src(s_nav_icon,arrow);
            s_nav_ui_changes++;
        }
        visible(s_nav_icon,true);
    }
    if(nav.distance_m!=s_last_distance) {
        s_nav_ui_changes++;
        char distance[24];
        if(nav.distance_m<1000U)
            (void)snprintf(distance,sizeof(distance),"%um",(unsigned)nav.distance_m);
        else if(nav.distance_m<10000U) {
            const unsigned km10=(nav.distance_m+50U)/100U;
            (void)snprintf(distance,sizeof(distance),"%u.%ukm",km10/10U,km10%10U);
        } else {
            const unsigned km=(nav.distance_m+500U)/1000U;
            (void)snprintf(distance,sizeof(distance),"%ukm",km);
        }
        /* Use the descriptors' true widths, not estimated character advances.
         * Width limiting is applied to all glyphs equally and only on changes. */
        const lv_image_dsc_t *assets[NAV_MAX_CHARS]={0};
        unsigned total_width=0;
        for(int i=0;i<NAV_MAX_CHARS && distance[i];++i){
            assets[i]=nav_glyph(distance[i]);
            if(assets[i]) total_width+=assets[i]->header.w;
        }
        const uint32_t scale=(total_width>NAV_DISTANCE_MAX_WIDTH) ?
            (NAV_DISTANCE_MAX_WIDTH*256U)/total_width : 256U;
        int x=NAV_DISTANCE_X;
        for(int i=0;i<NAV_MAX_CHARS;++i) {
            lv_obj_t *slot=s_nav_glyph[i];
            if(!slot) continue;
            const lv_image_dsc_t *im=assets[i];
            if(!im) {visible(slot,false);continue;}
            if(scale!=s_last_nav_scale) lv_image_set_scale(slot,scale);
            /* Distance changes frequently, but most digits remain the same.
             * Keep unchanged image sources and positions untouched. */
            if(s_nav_last_glyph[i]!=im) {
                lv_image_set_src(slot,im);
                s_nav_last_glyph[i]=im;
            }
            if(lv_obj_get_x(slot)!=x || lv_obj_get_y(slot)!=89)
                lv_obj_set_pos(slot,x,89);
            visible(slot,true);
            x+=(int)(((uint32_t)im->header.w*scale+128U)/256U);
        }
        s_last_nav_scale=scale;
        s_last_distance=nav.distance_m;
    }
    s_last_maneuver=nav.maneuver;
}

/* Important: do not call lv_image_set_src() once per 100-ms LVGL tick.
 * Such redundant writes force LCD invalidation and are a visible flicker risk. */
static void refresh_acc(uint32_t now_ms)
{
    const karoq_ui_acc_set_t state=karoq_ui_acc_set_snapshot(now_ms);
    if (state.signal_fresh) s_acc_last_fresh_age_ms=state.age_ms;
    else ++s_acc_stale_ticks;

    /* A stale/temporarily invalid snapshot is NOT evidence that the driver
     * switched ACC off. The RX task may update its timestamp after this UI
     * tick captured now_ms, and a 100-ms display tick can catch that gap.
     * A real, fresh OFF is confirmed sooner; a missing CAN signal is held
     * briefly, then reliably degrades to OFF. No indefinite latching. */
    const bool meaningful=state.valid && state.set_kmh<=999U &&
                          state.mode!=KAROQ_ACC_UI_OFF;
    if (!meaningful && s_acc_ui_drawn &&
        s_last_acc_mode!=KAROQ_ACC_UI_OFF) {
        if (!s_acc_bad_pending) {
            s_acc_bad_pending=true;
            s_acc_bad_start_ms=now_ms;
            ++s_acc_hold_events;
        }
        const uint32_t hold_ms=state.signal_fresh ?
            ACC_FRESH_OFF_CONFIRM_MS : ACC_STALE_HOLD_MS;
        if ((uint32_t)(now_ms-s_acc_bad_start_ms)<hold_ms) {
            ++s_acc_hold_ticks;
            return; /* Keep previously drawn gauge and SET unchanged. */
        }
        ++s_acc_off_confirmations;
    }
    s_acc_bad_pending=false;

    const karoq_acc_ui_mode_t display_mode=meaningful ?
        state.mode : KAROQ_ACC_UI_OFF;
    const uint16_t display_set=meaningful ? state.set_kmh : UINT16_MAX;
    if(s_acc_ui_drawn && display_mode==s_last_acc_mode &&
       display_set==s_last_set) return;

    ++s_acc_ui_changes;
    const bool mode_changed=!s_acc_ui_drawn || display_mode!=s_last_acc_mode;
    if(mode_changed) {
        ++s_acc_mode_changes;
        const bool active=display_mode==KAROQ_ACC_UI_ACTIVE;
        const bool previous_active=s_acc_ui_drawn &&
            s_last_acc_mode==KAROQ_ACC_UI_ACTIVE;
        const bool palette_changed=!s_acc_ui_drawn || active!=previous_active;
        /* OFF and STANDBY intentionally share the same gray speedometer.
         * Switching between them must not reload the same RGB565 image. */
        if(s_cruise_icon) {
            if(palette_changed) {
                lv_image_set_src(s_cruise_icon, active ?
                    &karoq_acc_new_green_cruise : &karoq_acc_new_gray_cruise);
                ++s_acc_icon_src_writes;
            }
            visible(s_cruise_icon,true);
        }
        visible(s_acc_active_dot,active);
        if(palette_changed) {
            if(s_set_label) lv_obj_set_style_text_color(s_set_label,
                lv_color_hex(active ? 0x20EF15 : 0xA6AEB8),0);
            if(s_set_unit) lv_obj_set_style_text_color(s_set_unit,
                lv_color_hex(active ? 0x20EF15 : 0xA6AEB8),0);
        }
    }
    if (display_set!=s_last_set) ++s_acc_set_changes;
    visible(s_set_label,meaningful);
    visible(s_set_unit,meaningful);
    if(!meaningful) {
        for(int i=0;i<SET_MAX_CHARS;++i)
            visible(s_set_digits[i],false);
    } else {
        char buf[8];
        (void)snprintf(buf,sizeof(buf),"%u",(unsigned)state.set_kmh);
        int n=0;
        while(buf[n] && n<SET_MAX_CHARS)n++;
        const int x=533+(3-n)*23;
        for(int i=0;i<SET_MAX_CHARS;++i) {
            lv_obj_t *slot=s_set_digits[i];
            if(!slot) continue;
            if(i>=n) {visible(slot,false);continue;}
            /* Rewrite digits only when the value/palette changed. */
            if(mode_changed || display_set!=s_last_set)
                lv_image_set_src(slot,set_glyph(buf[i],
                    display_mode==KAROQ_ACC_UI_ACTIVE));
            if (lv_obj_get_x(slot)!=x+i*23 || lv_obj_get_y(slot)!=121)
                lv_obj_set_pos(slot,x+i*23,121);
            visible(slot,true);
        }
    }
    s_last_set=display_set;
    s_last_acc_mode=display_mode;
    s_acc_ui_drawn=true;
}

void karoq_ref_layout_tick(uint32_t now_ms)
{
    if(!s_ready)return;
    karoq_oem_nav_tick(now_ms); /* V1.0.14 OEM NAV */
    observe_speed_positions();
    refresh_vza(now_ms);
    refresh_navigation(now_ms);
    refresh_acc(now_ms);
    if((uint32_t)(now_ms-s_ui_diag_ms)>=30000U) {
        /* If ACC is visibly blinking but ACC visual changes are zero,
         * investigate LVGL flush/panel power rather than ACC CAN decoder. */
        ESP_LOGI("KAROQ_UI_DIAG", "interval=30s acc_visual_changes=%lu nav_visual_changes=%lu speed_layout_mismatches=%lu",
                 (unsigned long)s_acc_ui_changes,
                 (unsigned long)s_nav_ui_changes,
                 (unsigned long)s_speed_pos_changes);
        ESP_LOGI("KAROQ_ACC_UI_DIAG", "interval=30s mode_changes=%lu set_changes=%lu icon_src_updates=%lu hold_events=%lu hold_ticks=%lu confirmed_off=%lu stale_ticks=%lu last_fresh_age_ms=%lu",
                 (unsigned long)s_acc_mode_changes,
                 (unsigned long)s_acc_set_changes,
                 (unsigned long)s_acc_icon_src_writes,
                 (unsigned long)s_acc_hold_events,
                 (unsigned long)s_acc_hold_ticks,
                 (unsigned long)s_acc_off_confirmations,
                 (unsigned long)s_acc_stale_ticks,
                 (unsigned long)s_acc_last_fresh_age_ms);
        s_ui_diag_ms=now_ms;
        s_acc_ui_changes=s_nav_ui_changes=s_speed_pos_changes=0;
        s_acc_mode_changes=s_acc_set_changes=s_acc_icon_src_writes=0;
        s_acc_hold_events=s_acc_hold_ticks=s_acc_off_confirmations=s_acc_stale_ticks=0;
    }
}
