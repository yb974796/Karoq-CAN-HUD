#include "karoq_ts_vza_decoder.h"
#include "karoq_ts_bap.h"
#include "karoq_ts_service.h"
#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

#define VZA_SIGN_TTL_MS 25000U
/* This version enables *trial* HUD display based on observed VZA/pACC correlation.
 * 0x02/0x20 semantics remain unknown; no safety/control use permitted.
 * Set to 0 to hide real VZA signs without affecting any original function.
 */
#ifndef KAROQ_TS_VZE_TRIAL_DISPLAY
#define KAROQ_TS_VZE_TRIAL_DISPLAY 1
#endif

static karoq_ts_bap_t s_bap;
static uint32_t s_rejected;

void karoq_ts_vza_reset(void)
{
    karoq_ts_bap_reset(&s_bap);
    s_rejected=0;
    karoq_ts_service_invalidate(KAROQ_TS_SOURCE_CAN_VZA);
}

uint32_t karoq_ts_vza_completed_count(void) {return s_bap.completed;}
uint32_t karoq_ts_vza_rejected_count(void) {return s_rejected;}

void karoq_ts_vza_receive(uint32_t can_id, bool extended, bool rtr,
                           const uint8_t *data, uint8_t dlc, uint32_t timestamp_ms)
{
    if (!extended || rtr || can_id != KAROQ_TS_BAP_CAN_ID || !data || dlc>8U) return;
    karoq_ts_bap_message_t msg;
    if (!karoq_ts_bap_feed(&s_bap,data,dlc,timestamp_ms,&msg)) return;
    /* 0x38: HeartbeatStatus, 0x48: Status; Function 0x50 on LSG 0x21.
     * The observed 18B payload has 01, speed_kmh, 00, 00, flags, ...
     * Flags 0x02 and 0x20 are empirically observed, NOT yet decoded.
     */
    if ((msg.opcode!=0x38U && msg.opcode!=0x48U) || msg.function!=0x50U ||
        msg.payload_len!=KAROQ_TS_BAP_APP_LEN) return;
    if (msg.payload[0]!=0x01U || msg.payload[2]!=0U || msg.payload[3]!=0U ||
        (msg.payload[4]!=0x02U && msg.payload[4]!=0x20U)) {
        ++s_rejected;
        karoq_ts_service_invalidate(KAROQ_TS_SOURCE_CAN_VZA);
        return;
    }
    /* Future vehicles may use other flags and speed sign types. Stop here safely. */
#ifdef ESP_PLATFORM
    const karoq_ts_snapshot_t previous = karoq_ts_service_snapshot(timestamp_ms);
#endif
    karoq_ts_event_t event = {
        .source=KAROQ_TS_SOURCE_CAN_VZA,
        .type=KAROQ_TS_EVENT_SPEED_LIMIT,
        .limit_kmh=msg.payload[1],
        .distance_m=-1,
        .timestamp_ms=timestamp_ms,
        .valid_ms=VZA_SIGN_TTL_MS,
        .raw_flags=msg.payload[4]
    };
#if KAROQ_TS_VZE_TRIAL_DISPLAY
    if (!karoq_ts_service_publish(&event)) {
        ++s_rejected;
        karoq_ts_service_invalidate(KAROQ_TS_SOURCE_CAN_VZA);
    } else {
#ifdef ESP_PLATFORM
        if (!previous.valid || previous.source != KAROQ_TS_SOURCE_CAN_VZA ||
            previous.limit_kmh != event.limit_kmh ||
            previous.raw_flags != event.raw_flags) {
            ESP_LOGI("KAROQ_VZA", "LIMIT=%u km/h flags=0x%02X BAP=0x%02X trial-only",
                (unsigned)event.limit_kmh,(unsigned)event.raw_flags,(unsigned)msg.opcode);
        }
#endif
    }
#else
    (void)event;
#endif
}
