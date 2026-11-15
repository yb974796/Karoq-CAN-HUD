#include "karoq_nav_carplay_adapter.h"
#include "karoq_nav_wire.h"

bool karoq_nav_carplay_maneuver(karoq_nav_maneuver_t maneuver,
                                uint32_t remaining_m, uint32_t local_now_ms,
                                uint32_t ttl_ms)
{
    const karoq_nav_event_t e = {
        .source = KAROQ_NAV_SRC_CARPLAY_RGI,
        .maneuver = maneuver,
        .distance_m = remaining_m,
        .timestamp_ms = local_now_ms,
        .valid_ms = ttl_ms
    };
    return karoq_nav_service_publish(&e);
}

void karoq_nav_carplay_disconnect(void)
{
    karoq_nav_wire_disconnect(KAROQ_NAV_SRC_CARPLAY_RGI);
}
