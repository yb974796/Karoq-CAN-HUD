#include "karoq_ts_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <string.h>

#define TS_MAX_TTL_MS 60000U
#define TS_MIN_TTL_MS 1000U

typedef struct { bool present; karoq_ts_event_t event; } ts_slot_t;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static ts_slot_t s_can, s_nav, s_camera;

static bool speed_plausible(uint16_t speed)
{
    /* Values outside this range or non-5km/h steps are not currently validated. */
    return speed >= 5U && speed <= 160U && speed % 5U == 0U;
}

bool karoq_ts_service_publish(const karoq_ts_event_t *event)
{
    if (!event || event->source <= KAROQ_TS_SOURCE_NONE ||
        event->source > KAROQ_TS_SOURCE_EXTERNAL_NAV ||
        event->valid_ms < TS_MIN_TTL_MS || event->valid_ms > TS_MAX_TTL_MS)
        return false;
    if (event->type == KAROQ_TS_EVENT_SPEED_LIMIT &&
        !speed_plausible(event->limit_kmh)) return false;
    if (event->type != KAROQ_TS_EVENT_SPEED_LIMIT &&
        event->type != KAROQ_TS_EVENT_LIMIT_END &&
        event->type != KAROQ_TS_EVENT_FIXED_CAMERA &&
        event->type != KAROQ_TS_EVENT_AVERAGE_CAMERA) return false;
    ts_slot_t *slot = NULL;
    if (event->type == KAROQ_TS_EVENT_FIXED_CAMERA ||
        event->type == KAROQ_TS_EVENT_AVERAGE_CAMERA) {
        if (event->source == KAROQ_TS_SOURCE_CAN_VZA) return false;
        if (event->distance_m < -1 || event->distance_m > 200000) return false;
        slot = &s_camera;
    } else {
        slot = event->source == KAROQ_TS_SOURCE_CAN_VZA ? &s_can : &s_nav;
    }
    portENTER_CRITICAL(&s_mux);
    slot->event = *event;
    slot->present = true;
    portEXIT_CRITICAL(&s_mux);
    return true;
}

void karoq_ts_service_invalidate(karoq_ts_source_t source)
{
    portENTER_CRITICAL(&s_mux);
    if (source == KAROQ_TS_SOURCE_CAN_VZA) s_can.present = false;
    if (source == KAROQ_TS_SOURCE_CARPLAY || source == KAROQ_TS_SOURCE_EXTERNAL_NAV) {
        if (s_nav.present && s_nav.event.source == source) s_nav.present = false;
        if (s_camera.present && s_camera.event.source == source) s_camera.present = false;
    }
    portEXIT_CRITICAL(&s_mux);
}

static bool is_fresh(const ts_slot_t *slot, uint32_t now)
{
    if (!slot->present) return false;
    /* UINT32 subtraction handles uptime wrap, not out-of-order messages.
     * A future timestamp from malformed caller is rejected via half-range rule.
     */
    uint32_t age = now - slot->event.timestamp_ms;
    return age <= slot->event.valid_ms;
}

karoq_ts_snapshot_t karoq_ts_service_snapshot(uint32_t now_ms)
{
    karoq_ts_snapshot_t out = {0};
    ts_slot_t can,nav,camera;
    portENTER_CRITICAL(&s_mux);
    can=s_can; nav=s_nav; camera=s_camera;
    portEXIT_CRITICAL(&s_mux);
    /* CAN has priority; invalid/end-of-limit states must explicitly hide sign. */
    const ts_slot_t *chosen = is_fresh(&can,now_ms) ? &can :
                              (is_fresh(&nav,now_ms) ? &nav : NULL);
    if (chosen && chosen->event.type == KAROQ_TS_EVENT_SPEED_LIMIT) {
        out.valid=true;
        out.limit_kmh=chosen->event.limit_kmh;
        out.source=chosen->event.source;
        out.raw_flags=chosen->event.raw_flags;
        out.age_ms=now_ms-chosen->event.timestamp_ms;
    }
    if (is_fresh(&camera,now_ms)) {
        out.camera_valid=true;
        out.camera_type=camera.event.type;
        out.camera_distance_m=camera.event.distance_m;
        out.camera_source=camera.event.source;
    }
    return out;
}
