#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Source-independent HUD traffic model. Not used for vehicle control.
 * CAN VZA takes priority over external navigation while fresh.
 * Camera alerts are separate from speed signs and disabled by default.
 */
typedef enum {
    KAROQ_TS_SOURCE_NONE = 0,
    KAROQ_TS_SOURCE_CAN_VZA = 1,
    KAROQ_TS_SOURCE_CARPLAY = 2,
    KAROQ_TS_SOURCE_EXTERNAL_NAV = 3
} karoq_ts_source_t;

typedef enum {
    KAROQ_TS_EVENT_SPEED_LIMIT = 1,
    KAROQ_TS_EVENT_LIMIT_END = 2,
    KAROQ_TS_EVENT_FIXED_CAMERA = 3,
    KAROQ_TS_EVENT_AVERAGE_CAMERA = 4
} karoq_ts_event_type_t;

typedef struct {
    karoq_ts_source_t source;
    karoq_ts_event_type_t type;
    uint16_t limit_kmh; /* 0 for limit end / camera events */
    int32_t distance_m; /* -1 = unavailable; reserved for future UI */
    uint32_t timestamp_ms;
    uint32_t valid_ms;
    uint8_t raw_flags;
} karoq_ts_event_t;

typedef struct {
    bool valid;
    uint16_t limit_kmh;
    karoq_ts_source_t source;
    uint8_t raw_flags;
    uint32_t age_ms;
    bool camera_valid;
    karoq_ts_event_type_t camera_type;
    int32_t camera_distance_m;
    karoq_ts_source_t camera_source;
} karoq_ts_snapshot_t;

bool karoq_ts_service_publish(const karoq_ts_event_t *event);
void karoq_ts_service_invalidate(karoq_ts_source_t source);
karoq_ts_snapshot_t karoq_ts_service_snapshot(uint32_t now_ms);
