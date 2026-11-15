#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Navigation *display information*, never used for vehicle control.
 * Integrators must normalize actual route-guidance packets on the trusted
 * MHI2/MST2 / phone side; this is not an iAP2 parser or CarPlay speed API.
 */
typedef enum {
    KAROQ_NAV_SRC_NONE = 0,
    KAROQ_NAV_SRC_CARPLAY_RGI = 1,
    KAROQ_NAV_SRC_HEADUNIT = 2,
    KAROQ_NAV_SRC_EXTERNAL = 3
} karoq_nav_source_t;

typedef enum {
    KAROQ_NAV_NONE = 0,
    KAROQ_NAV_STRAIGHT = 1,
    KAROQ_NAV_LEFT = 2,
    KAROQ_NAV_RIGHT = 3,
    KAROQ_NAV_SLIGHT_LEFT = 4,
    KAROQ_NAV_SLIGHT_RIGHT = 5,
    KAROQ_NAV_SHARP_LEFT = 6,
    KAROQ_NAV_SHARP_RIGHT = 7,
    KAROQ_NAV_UTURN = 8,
    KAROQ_NAV_ROUNDABOUT = 9,
    KAROQ_NAV_EXIT_LEFT = 10,
    KAROQ_NAV_EXIT_RIGHT = 11,
    KAROQ_NAV_UTURN_LEFT = 12,
    KAROQ_NAV_UTURN_RIGHT = 13
} karoq_nav_maneuver_t;

typedef struct {
    karoq_nav_source_t source;
    karoq_nav_maneuver_t maneuver;
    uint32_t distance_m;
    uint32_t timestamp_ms; /* ESP32 monotonic clock, not phone wall time */
    uint32_t valid_ms;
} karoq_nav_event_t;

typedef struct {
    bool valid;
    karoq_nav_source_t source;
    karoq_nav_maneuver_t maneuver;
    uint32_t distance_m;
    uint32_t age_ms;
} karoq_nav_snapshot_t;

bool karoq_nav_service_publish(const karoq_nav_event_t *event);
void karoq_nav_service_invalidate(karoq_nav_source_t source);
karoq_nav_snapshot_t karoq_nav_service_snapshot(uint32_t now_ms);
