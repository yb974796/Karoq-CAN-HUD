#include "karoq_oem_nav_decoder.h"
#include "karoq_oem_nav_bap.h"
#include "karoq_nav_service.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <limits.h>
#include <stddef.h>

/* V1.0.14.1: genuine MHI2 route observed on CAN. Validate live-only
 * turn+distance; never write vehicle control. CANLOG showed Navigation_SD
 * function 0x12 payload[0..3] in LITTLE-ENDIAN, not network order.
 */
#define OEM_NAV_FRESH_MS 45000U /* > observed 30,003ms heartbeat */
#define OEM_NAV_RENEW_MS 2000U
#define OEM_NAV_SERVICE_TTL_MS 12000U
#define OEM_NAV_INVALID_GRACE_MS 1200U /* bridge asynchronous BAP fields */

static const char *TAG="KAROQ_OEM_NAV";
static karoq_oem_bap_t s_bap;
static portMUX_TYPE s_mux=portMUX_INITIALIZER_UNLOCKED;
static bool s_rg_active;
static bool s_rg_seen;
static uint32_t s_rg_ms;
static bool s_arrow_valid;
static bool s_arrow_seen;
static karoq_nav_maneuver_t s_arrow;
static uint32_t s_arrow_ms;
static bool s_distance_valid;
static bool s_distance_seen;
static uint32_t s_distance_m;
static uint32_t s_distance_ms;
static uint32_t s_last_renew_ms;
static uint32_t s_arrow_bad_since_ms;
static uint32_t s_distance_bad_since_ms;
static bool s_invalid_cleared;

static karoq_nav_maneuver_t direction(uint8_t d)
{
    if (d<=16U || d>=240U) return KAROQ_NAV_STRAIGHT;
    if (d<48U) return KAROQ_NAV_SLIGHT_LEFT;
    if (d<80U) return KAROQ_NAV_LEFT;
    if (d<111U) return KAROQ_NAV_SHARP_LEFT;
    if (d<145U) return KAROQ_NAV_UTURN;
    if (d<176U) return KAROQ_NAV_SHARP_RIGHT;
    if (d<209U) return KAROQ_NAV_RIGHT;
    return KAROQ_NAV_SLIGHT_RIGHT;
}

static karoq_nav_maneuver_t maneuver(uint8_t main_element,uint8_t dir)
{
    switch (main_element) {
        case 0x0BU: return KAROQ_NAV_STRAIGHT; /* FollowStreet */
        case 0x0DU: /* Turn */
        case 0x0EU: /* TurnOnMainroad */
        case 0x13U: /* Fork-2 */
        case 0x14U: /* Fork-3 */
        case 0x1CU: /* PrepareTurn */
            return direction(dir);
        case 0x0FU: /* ExitRight */
        case 0x11U: /* ServiceRoadRight */
            return KAROQ_NAV_EXIT_RIGHT;
        case 0x10U: /* ExitLeft */
        case 0x12U: /* ServiceRoadLeft */
            return KAROQ_NAV_EXIT_LEFT;
        case 0x19U: /* Uturn, directional BAP field */
            /* Actual Karoq car + HUD photo: direction=0x40 => LEFT U-turn.
             * We have no verified RIGHT-U-turn packet yet. Remain unknown,
             * rather than showing a right U-turn at a left U-turn junction. */
            if(dir==0x40U) return KAROQ_NAV_UTURN_LEFT;
            return KAROQ_NAV_NONE;
        default:
            /* Roundabout/complex maneuvers omitted rather than misdrawn
             * as U-turn by existing reference-layout assets. */
            return KAROQ_NAV_NONE;
    }
}

static bool fresh(uint32_t now,uint32_t stamp)
{
    return (uint32_t)(now-stamp)<=OEM_NAV_FRESH_MS;
}

static void publish(uint32_t now_ms)
{
    karoq_nav_maneuver_t arrow=KAROQ_NAV_NONE;
    uint32_t dist=0;
    bool ready=false;
    portENTER_CRITICAL(&s_mux);
    if (s_rg_active && s_arrow_valid && s_distance_valid &&
        fresh(now_ms,s_rg_ms) && fresh(now_ms,s_arrow_ms) &&
        fresh(now_ms,s_distance_ms) &&
        (uint32_t)(now_ms-s_last_renew_ms)>=OEM_NAV_RENEW_MS) {
        ready=true;arrow=s_arrow;dist=s_distance_m;
        s_last_renew_ms=now_ms;
    }
    portEXIT_CRITICAL(&s_mux);
    if (!ready) return;
    const karoq_nav_event_t ev={
        .source=KAROQ_NAV_SRC_HEADUNIT,
        .maneuver=arrow,
        .distance_m=dist,
        .timestamp_ms=now_ms,
        .valid_ms=OEM_NAV_SERVICE_TTL_MS,
    };
    (void)karoq_nav_service_publish(&ev);
}

void karoq_oem_nav_receive(uint32_t can_id,bool extended,bool rtr,
                           const uint8_t *data,uint8_t dlc,uint32_t now_ms)
{
    if (!extended || rtr || !data || dlc==0U || dlc>8U) return;
    if (can_id!=0x17333210UL && can_id!=0x17333211UL) return;
    karoq_oem_bap_message_t m;
    /* Receive is always dispatched from the single existing TWAI task. */
    if (!karoq_oem_bap_feed(&s_bap,can_id,data,dlc,now_ms,&m)) return;
    if (m.lsg!=0x32U || (m.opcode!=3U && m.opcode!=4U)) return;

    if (m.function_id==0x11U && m.payload_len==1U) {
        const uint8_t rg=m.payload[0];
        if (rg!=0U && rg!=1U && rg!=2U) return;
        bool changed;
        portENTER_CRITICAL(&s_mux);
        changed=(!s_rg_seen || s_rg_active != (rg==1U));
        s_rg_seen=true;
        s_rg_active=(rg==1U);
        s_rg_ms=now_ms;
        if (rg!=1U) {
            s_arrow_valid=false;s_distance_valid=false;
            s_invalid_cleared=false;
        }
        portEXIT_CRITICAL(&s_mux);
        if (rg!=1U) karoq_nav_service_invalidate(KAROQ_NAV_SRC_HEADUNIT);
        if (changed) ESP_LOGI(TAG,"OEM RG=%u (1=active, 0=off, 2=suspended)",(unsigned)rg);
    } else if (m.function_id==0x12U && m.payload_len==8U) {
        /* Navigation_SD Function 0x12: little-endian uint32 distance.
         * CANLOG: 40 1F 00 00 00 = 800.0m, unit 0.
         *         0A 00 00 00 01 = 1000m, unit 1. */
        const uint32_t raw=(uint32_t)m.payload[0] |
            ((uint32_t)m.payload[1]<<8) |
            ((uint32_t)m.payload[2]<<16) |
            ((uint32_t)m.payload[3]<<24);
        uint32_t distance_m=0;
        const uint8_t unit=m.payload[4];
        bool ok=(m.payload[7]&1U)!=0U && raw!=UINT32_MAX;
        if (ok) {
            if (unit==0U) distance_m=raw/10U; /* 0.1 metre units */
            else if (unit==1U && raw<=20000U) distance_m=raw*100U; /* 0.1 km */
            else ok=false;
        }
        if (distance_m>2000000U) ok=false;
        bool changed;
        portENTER_CRITICAL(&s_mux);
        changed=(!s_distance_seen || s_distance_valid!=ok || (ok && s_distance_m!=distance_m));
        const bool was_valid=s_distance_valid;
        s_distance_seen=true;
        s_distance_valid=ok;
        s_distance_ms=now_ms;
        if (ok) {
            s_distance_m=distance_m;
            /* Publish genuine value changes immediately, even if the
             * 2-second heartbeat throttle has not elapsed. */
            if (changed) s_last_renew_ms=now_ms-OEM_NAV_RENEW_MS;
            if (s_arrow_valid) s_invalid_cleared=false;
        } else if(was_valid) s_distance_bad_since_ms=now_ms;
        portEXIT_CRITICAL(&s_mux);
        if (changed && ok)
            ESP_LOGI(TAG,"OEM NEXT distance=%lu m (unit=%u)",
                     (unsigned long)distance_m,(unsigned)unit);
        else if (changed) {
            ESP_LOGI(TAG,"OEM NEXT invalid (flags=0x%02X unit=0x%02X)",
                     (unsigned)m.payload[7],(unsigned)m.payload[4]);
            /* Not a route-stop: keep last good indication briefly while
             * asynchronously transmitted distance/arrow fields catch up. */
        }
    } else if (m.function_id==0x17U && m.payload_len>=4U) {
        const karoq_nav_maneuver_t arrow=maneuver(m.payload[0],m.payload[1]);
        const bool ok=(arrow!=KAROQ_NAV_NONE);
        bool changed;
        portENTER_CRITICAL(&s_mux);
        changed=(!s_arrow_seen || s_arrow_valid!=ok || (ok && s_arrow!=arrow));
        const bool was_valid=s_arrow_valid;
        s_arrow_seen=true;
        s_arrow_valid=ok;
        s_arrow=arrow;
        s_arrow_ms=now_ms;
        if(ok) {
            if (changed) s_last_renew_ms=now_ms-OEM_NAV_RENEW_MS;
            if (s_distance_valid) s_invalid_cleared=false;
        } else if(was_valid) s_arrow_bad_since_ms=now_ms;
        portEXIT_CRITICAL(&s_mux);
        if (changed)
            ESP_LOGI(TAG,"OEM ARROW main=0x%02X direction=0x%02X mapped=%u",
                     (unsigned)m.payload[0],(unsigned)m.payload[1],(unsigned)arrow);
        /* Unknown intermediate BAP geometry must not clear the previous
         * maneuver immediately; it is cleared after a bounded grace period. */
    }
    publish(now_ms);
}

void karoq_oem_nav_tick(uint32_t now_ms)
{
    bool expired;
    bool invalid_grace_expired=false;
    portENTER_CRITICAL(&s_mux);
    expired=s_rg_active && (!fresh(now_ms,s_rg_ms) ||
              (s_arrow_valid && !fresh(now_ms,s_arrow_ms)) ||
              (s_distance_valid && !fresh(now_ms,s_distance_ms)));
    if(expired) {
        s_rg_active=false;s_arrow_valid=false;s_distance_valid=false;
        s_invalid_cleared=false;
    } else if (s_rg_active) {
        const bool arrow_bad=s_arrow_seen && !s_arrow_valid &&
            (uint32_t)(now_ms-s_arrow_bad_since_ms)>OEM_NAV_INVALID_GRACE_MS;
        const bool dist_bad=s_distance_seen && !s_distance_valid &&
            (uint32_t)(now_ms-s_distance_bad_since_ms)>OEM_NAV_INVALID_GRACE_MS;
        if ((arrow_bad || dist_bad) && !s_invalid_cleared) {
            s_invalid_cleared=true;
            invalid_grace_expired=true;
        }
    }
    portEXIT_CRITICAL(&s_mux);
    if(expired || invalid_grace_expired)
        karoq_nav_service_invalidate(KAROQ_NAV_SRC_HEADUNIT);
    else publish(now_ms);
}
