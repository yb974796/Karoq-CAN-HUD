#include "karoq_vze_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

/* Until an actual A5/17/5F VZE field is correlated to ground truth,
 * do not allow arbitrary candidate bytes to reach the speed-sign widget.
 */
#ifndef KAROQ_VZE_SOURCE_VERIFIED
#define KAROQ_VZE_SOURCE_VERIFIED 0
#endif
#define KAROQ_VZE_FRESH_MS 2500U

static portMUX_TYPE s_vze_mux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t s_limit;
static uint32_t s_timestamp;
static bool s_present;

bool karoq_vze_state_commit_verified(uint16_t limit_kmh, uint32_t uptime_ms)
{
#if KAROQ_VZE_SOURCE_VERIFIED
    /* A sanity gate, NOT a traffic-sign decoder. Change after ground truth.
     * Never infer current road speed limit from ESP speed or ACC set speed.
     */
    if (limit_kmh < 5U || limit_kmh > 140U || (limit_kmh % 5U) != 0U) return false;
    portENTER_CRITICAL(&s_vze_mux);
    s_limit = limit_kmh;
    s_timestamp = uptime_ms;
    s_present = true;
    portEXIT_CRITICAL(&s_vze_mux);
    return true;
#else
    (void)limit_kmh;
    (void)uptime_ms;
    return false;
#endif
}

void karoq_vze_state_clear(void)
{
    portENTER_CRITICAL(&s_vze_mux);
    s_present = false;
    s_limit = 0U;
    s_timestamp = 0U;
    portEXIT_CRITICAL(&s_vze_mux);
}

karoq_vze_snapshot_t karoq_vze_state_get(uint32_t uptime_ms)
{
    karoq_vze_snapshot_t out = {0};
    bool seen;
    uint16_t limit;
    uint32_t timestamp;
    portENTER_CRITICAL(&s_vze_mux);
    seen = s_present;
    limit = s_limit;
    timestamp = s_timestamp;
    portEXIT_CRITICAL(&s_vze_mux);
    if (!seen) return out;
    /* Standard unsigned arithmetic works across one 32-bit uptime wrap.
     * An out-of-order timestamp fails closed rather than displaying stale data.
     */
    uint32_t age = uptime_ms - timestamp;
    if (age > KAROQ_VZE_FRESH_MS) return out;
    out.valid = true;
    out.limit_kmh = limit;
    out.age_ms = age;
    return out;
}
