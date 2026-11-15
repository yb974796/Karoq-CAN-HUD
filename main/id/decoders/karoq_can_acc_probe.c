#include "karoq_can_acc_probe.h"
#include <string.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_timer.h"
#endif

// VW MQB DBC: BO_ 780 ACC_02, CAN STD 0x30C
// Target speed: start bit 12, length 10, little endian, scale 0.32 km/h.
// Display state: start bit 61, length 3. Primary state: bit22, length2.
// Status numeric values are intentionally not mapped to engaged/disengaged.
bool karoq_acc_decode(const karoq_can_frame_t *frame, karoq_acc_snapshot_t *out)
{
    if (!frame || !out || frame->extended || frame->rtr || frame->id != 0x30CU || frame->dlc < 8U) return false;
    memset(out, 0, sizeof(*out));
    memcpy(out->data, frame->data, 8U);
    out->target_raw = (uint16_t)(frame->data[1] >> 4) | ((uint16_t)(frame->data[2] & 0x3FU) << 4);
    out->target_kmh = 0.32f * (float)out->target_raw;
    out->target_valid = out->target_raw != 0x3FFU && out->target_kmh <= 320.0f;
    out->status_primary = (frame->data[2] >> 6) & 0x03U;
    out->status_display = (frame->data[7] >> 5) & 0x07U;
    out->valid = true;
    return true;
}

#ifdef ESP_PLATFORM
#define KAROQ_ACC_STALE_MS 750U
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static karoq_acc_snapshot_t s_last = {0};
static uint64_t s_updated_ms = 0;
static bool s_seen = false;

void karoq_acc_ingest(const karoq_can_frame_t *frame)
{
    karoq_acc_snapshot_t sample;
    if (!karoq_acc_decode(frame, &sample)) return;
    portENTER_CRITICAL(&s_mux);
    // Ignore rolling checksum/counter changes (bytes 0, and byte 1 low nibble).
    bool meaningful = !s_seen ||
        s_last.status_display != sample.status_display ||
        s_last.status_primary != sample.status_primary ||
        s_last.target_raw != sample.target_raw ||
        memcmp(&s_last.data[3], &sample.data[3], 4U) != 0 ||
        ((s_last.data[1] ^ sample.data[1]) & 0xF0U) != 0 ||
        ((s_last.data[7] ^ sample.data[7]) & 0x1FU) != 0;
    sample.seq = s_last.seq + (meaningful ? 1U : 0U);
    s_last = sample;
    s_updated_ms = frame->timestamp_ms;
    s_seen = true;
    portEXIT_CRITICAL(&s_mux);
}

karoq_acc_snapshot_t karoq_acc_get(void)
{
    karoq_acc_snapshot_t snap;
    uint64_t timestamp;
    bool seen;
    portENTER_CRITICAL(&s_mux);
    snap = s_last;
    timestamp = s_updated_ms;
    seen = s_seen;
    portEXIT_CRITICAL(&s_mux);
    uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000ULL;
    snap.age_ms = (now_ms >= timestamp) ? (uint32_t)(now_ms - timestamp) : UINT32_MAX;
    snap.valid = seen && (snap.age_ms <= KAROQ_ACC_STALE_MS);
    return snap;
}
#else
void karoq_acc_ingest(const karoq_can_frame_t *frame) { (void)frame; }
karoq_acc_snapshot_t karoq_acc_get(void) { karoq_acc_snapshot_t s = {0}; return s; }
#endif
