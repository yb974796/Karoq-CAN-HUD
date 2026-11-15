#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Distinct enum from both MQB opendbc and original HUD; no name collisions. */
typedef enum {
    KAROQ_LINK_UNKNOWN = 0,
    KAROQ_LINK_OFF,
    KAROQ_LINK_ACC_ONLY,
    KAROQ_LINK_LANE,
    KAROQ_LINK_TRAVEL
} karoq_link_assist_t;

typedef struct {
    bool speed_valid;
    uint16_t speed_kph;
    karoq_link_assist_t assist;
    uint8_t tsk_status;
    bool acc_known;
    bool lane_candidate; /* not authoritative */
    bool travel_candidate; /* not authoritative */
} karoq_v06_live_model_t;

/* No LVGL calls, pure snapshot: UI task may call safely. */
karoq_v06_live_model_t karoq_v06_live_model_read(void);
