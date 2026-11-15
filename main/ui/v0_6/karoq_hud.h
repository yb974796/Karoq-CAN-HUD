#pragma once
/* Public contract of the original Karoq CAN HUD V0.6 UI.
 * This header accompanies the actual source provided by the user.
 * NO other UI implementations or overlays are permitted in V1.0.8.
 */
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    KAROQ_ASSIST_UNKNOWN = 0,
    KAROQ_ASSIST_OFF,
    KAROQ_ASSIST_LANE_ACTIVE,
    KAROQ_ASSIST_TRAVEL_ACTIVE,
    KAROQ_ASSIST_ACC_ONLY
} karoq_assist_state_t;

typedef enum {
    KAROQ_SOURCE_NONE = 0,
    KAROQ_SOURCE_DEMO,
    KAROQ_SOURCE_CAN_VERIFIED
} karoq_hud_source_t;

typedef struct {
    karoq_assist_state_t assist_state;
    bool speed_valid;
    uint16_t speed_kph;
    karoq_hud_source_t source;
} karoq_hud_state_t;

void karoq_hud_ui_init(void);
/* Invoke only in LVGL context / while holding project's LVGL lock. */
void karoq_hud_ui_set_state(const karoq_hud_state_t *state);
