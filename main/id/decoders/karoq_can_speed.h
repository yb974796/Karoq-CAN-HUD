#pragma once
#include "karoq_can_types.h"
#ifdef __cplusplus
extern "C" {
#endif
// Signal meaning verified against commaai/opendbc/opendbc/dbc/vw_mqb.dbc.
// Moving Karoq vehicle comparison and ESP QBit polarity not yet verified.
typedef enum {
    KAROQ_SPEED_NONE = 0,
    KAROQ_SPEED_ESP_0FD,
    KAROQ_SPEED_CLUSTER_30B,
} karoq_speed_source_t;
typedef struct {
    bool valid;
    float kmh;
    karoq_speed_source_t source;
    uint16_t raw;
    uint32_t age_ms;
    bool esp_qbit;   // Report only: polarity not confirmed on this car
} karoq_speed_snapshot_t;

// Pure DBC decoder. No ESP-IDF or LVGL dependency. 0.01 km/h on ESP_21;
// 0.32 km/h on Kombi_01. Reject implausible (>320 km/h) raw values.
bool karoq_speed_decode(const karoq_can_frame_t *frame, karoq_speed_snapshot_t *out);

// Thread-safe ingestion from CAN task; snapshot from LVGL task.
void karoq_speed_ingest(const karoq_can_frame_t *frame);
karoq_speed_snapshot_t karoq_speed_get(void);
const char *karoq_speed_source_name(karoq_speed_source_t src);
#ifdef __cplusplus
}
#endif
