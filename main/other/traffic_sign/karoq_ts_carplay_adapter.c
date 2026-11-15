#include "karoq_ts_carplay_adapter.h"
#include "karoq_ts_service.h"

bool karoq_ts_carplay_speed_limit(uint16_t kmh, uint32_t timestamp_ms, uint32_t ttl_ms)
{
    const karoq_ts_event_t e={
        .source=KAROQ_TS_SOURCE_CARPLAY, .type=KAROQ_TS_EVENT_SPEED_LIMIT,
        .limit_kmh=kmh, .distance_m=-1,
        .timestamp_ms=timestamp_ms, .valid_ms=ttl_ms, .raw_flags=0
    };
    return karoq_ts_service_publish(&e);
}

bool karoq_ts_carplay_camera(bool average, int32_t distance_m,
                             uint32_t timestamp_ms, uint32_t ttl_ms)
{
    const karoq_ts_event_t e={
        .source=KAROQ_TS_SOURCE_CARPLAY,
        .type=average ? KAROQ_TS_EVENT_AVERAGE_CAMERA : KAROQ_TS_EVENT_FIXED_CAMERA,
        .distance_m=distance_m,
        .timestamp_ms=timestamp_ms, .valid_ms=ttl_ms,
    };
    return karoq_ts_service_publish(&e);
}

void karoq_ts_carplay_disconnect(void)
{
    karoq_ts_service_invalidate(KAROQ_TS_SOURCE_CARPLAY);
}
