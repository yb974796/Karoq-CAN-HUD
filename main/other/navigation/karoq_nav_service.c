#include "karoq_nav_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <string.h>

#define NAV_MIN_TTL_MS 500U
#define NAV_MAX_TTL_MS 15000U
#define NAV_MAX_DISTANCE_M 2000000U

typedef struct { bool present; karoq_nav_event_t event; } nav_slot_t;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
/* One independent slot for each potential provider. */
static nav_slot_t s_slots[4];

static bool supported_source(karoq_nav_source_t src)
{
    return src >= KAROQ_NAV_SRC_CARPLAY_RGI && src <= KAROQ_NAV_SRC_EXTERNAL;
}

bool karoq_nav_service_publish(const karoq_nav_event_t *event)
{
    if (!event || !supported_source(event->source) ||
        event->maneuver < KAROQ_NAV_STRAIGHT ||
        event->maneuver > KAROQ_NAV_EXIT_RIGHT ||
        event->distance_m > NAV_MAX_DISTANCE_M ||
        event->valid_ms < NAV_MIN_TTL_MS ||
        event->valid_ms > NAV_MAX_TTL_MS) return false;

    bool ok = true;
    portENTER_CRITICAL(&s_mux);
    nav_slot_t *slot = &s_slots[event->source];
    if (slot->present) {
        /* Reject duplicate / older source-local samples (wrap-safe half range). */
        const uint32_t delta = event->timestamp_ms - slot->event.timestamp_ms;
        if (delta == 0 || delta >= 0x80000000U) ok = false;
    }
    if (ok) { slot->event = *event; slot->present = true; }
    portEXIT_CRITICAL(&s_mux);
    return ok;
}

void karoq_nav_service_invalidate(karoq_nav_source_t source)
{
    if (!supported_source(source)) return;
    portENTER_CRITICAL(&s_mux);
    s_slots[source].present = false;
    portEXIT_CRITICAL(&s_mux);
}

static bool fresh(const nav_slot_t *s, uint32_t now_ms)
{
    return s->present && ((uint32_t)(now_ms - s->event.timestamp_ms) <= s->event.valid_ms);
}

karoq_nav_snapshot_t karoq_nav_service_snapshot(uint32_t now_ms)
{
    nav_slot_t slots[4];
    karoq_nav_snapshot_t out = {0};
    portENTER_CRITICAL(&s_mux);
    memcpy(slots, s_slots, sizeof(slots));
    portEXIT_CRITICAL(&s_mux);
    /* Source precedence; never merge two routes, nor modify original VZA/pACC. */
    const unsigned priority[] = {KAROQ_NAV_SRC_CARPLAY_RGI, KAROQ_NAV_SRC_HEADUNIT,
                                 KAROQ_NAV_SRC_EXTERNAL};
    for (unsigned i = 0; i < sizeof(priority)/sizeof(priority[0]); ++i) {
        const nav_slot_t *s = &slots[priority[i]];
        if (fresh(s, now_ms)) {
            out.valid = true;
            out.source = s->event.source;
            out.maneuver = s->event.maneuver;
            out.distance_m = s->event.distance_m;
            out.age_ms = now_ms - s->event.timestamp_ms;
            break;
        }
    }
    return out;
}
