#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Input adapter ONLY. No CarPlay/iAP2 interception or transport implemented.
 * A future trusted app/bridge may call this API with documented data provenance.
 */
bool karoq_ts_carplay_speed_limit(uint16_t kmh, uint32_t timestamp_ms, uint32_t ttl_ms);
bool karoq_ts_carplay_camera(bool average, int32_t distance_m,
                             uint32_t timestamp_ms, uint32_t ttl_ms);
void karoq_ts_carplay_disconnect(void);
