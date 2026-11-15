#include "karoq_can_speed.h"
#include <string.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_timer.h"
#endif

// Verified DBC: BO_ 253 ESP_21, ESP_v_Signal : 32|16@1+ (0.01,0) km/h
// Verified DBC: BO_ 779 Kombi_01, KBI_angez_Geschw : 48|10@1+ (0.32,0) km/h
bool karoq_speed_decode(const karoq_can_frame_t *frame, karoq_speed_snapshot_t *out)
{
    if (!frame || !out || frame->extended || frame->rtr || frame->dlc < 8) return false;
    memset(out, 0, sizeof(*out));
    uint16_t raw;
    if (frame->id == 0x0FD) {
        raw = (uint16_t)frame->data[4] | ((uint16_t)frame->data[5] << 8);
        out->kmh = raw * 0.01f;
        out->source = KAROQ_SPEED_ESP_0FD;
        // ESP_QBit_v_Signal = DBC bit 55 = byte 6 bit 7.
        // QBit meaning/polarity deliberately NOT used to reject values yet.
        out->esp_qbit = (frame->data[6] & 0x80U) != 0;
    } else if (frame->id == 0x30B) {
        raw = (uint16_t)frame->data[6] | (((uint16_t)frame->data[7] & 0x03U) << 8);
        out->kmh = raw * 0.32f;
        out->source = KAROQ_SPEED_CLUSTER_30B;
    } else return false;
    if (out->kmh > 320.0f) return false;
    out->raw = raw;
    out->valid = true;
    return true;
}

#ifdef ESP_PLATFORM
#define KAROQ_SPEED_STALE_MS 750ULL
static portMUX_TYPE s_speed_mux = portMUX_INITIALIZER_UNLOCKED;
typedef struct {
    bool seen;
    karoq_speed_snapshot_t data;
    uint64_t updated_ms;
} speed_slot_t;
static speed_slot_t s_esp = {0}, s_cluster = {0};

void karoq_speed_ingest(const karoq_can_frame_t *frame)
{
    karoq_speed_snapshot_t sample;
    if (!karoq_speed_decode(frame, &sample)) return;
    portENTER_CRITICAL(&s_speed_mux);
    speed_slot_t *slot = (sample.source == KAROQ_SPEED_ESP_0FD) ? &s_esp : &s_cluster;
    slot->seen = true;
    slot->data = sample;
    slot->updated_ms = frame->timestamp_ms;
    portEXIT_CRITICAL(&s_speed_mux);
}

karoq_speed_snapshot_t karoq_speed_get(void)
{
    speed_slot_t esp, cluster;
    portENTER_CRITICAL(&s_speed_mux);
    esp = s_esp;
    cluster = s_cluster;
    portEXIT_CRITICAL(&s_speed_mux);
    uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000ULL;
    speed_slot_t *selected = NULL;
    if (esp.seen && now_ms >= esp.updated_ms && now_ms - esp.updated_ms <= KAROQ_SPEED_STALE_MS) selected = &esp;
    else if (cluster.seen && now_ms >= cluster.updated_ms && now_ms - cluster.updated_ms <= KAROQ_SPEED_STALE_MS) selected = &cluster;
    karoq_speed_snapshot_t result = {0};
    if (selected) {
        result = selected->data;
        result.age_ms = (uint32_t)(now_ms - selected->updated_ms);
    }
    return result;
}
#else
// Host unit test builds only the pure decoder.
void karoq_speed_ingest(const karoq_can_frame_t *f) {(void)f;}
karoq_speed_snapshot_t karoq_speed_get(void) {karoq_speed_snapshot_t s={0}; return s;}
#endif

const char *karoq_speed_source_name(karoq_speed_source_t src)
{
    switch (src) {
    case KAROQ_SPEED_ESP_0FD: return "ESP_21/0x0FD";
    case KAROQ_SPEED_CLUSTER_30B: return "Kombi_01/0x30B";
    default: return "NO_SIGNAL";
    }
}
