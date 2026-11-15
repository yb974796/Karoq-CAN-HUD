#pragma once
#include <stdbool.h>
#include <stdint.h>

/* VZE/traffic-sign module. No guessed CAN ID or value-to-speed mapping.
 * The receiving CAN parser MUST remain passive/listen-only.
 * Production defaults to KAROQ_VZE_SOURCE_VERIFIED=0 and shows NO SIGN.
 */
typedef struct {
    bool valid;
    uint16_t limit_kmh;
    uint32_t age_ms;
} karoq_vze_snapshot_t;

bool karoq_vze_state_commit_verified(uint16_t limit_kmh, uint32_t uptime_ms);
void karoq_vze_state_clear(void);
karoq_vze_snapshot_t karoq_vze_state_get(uint32_t uptime_ms);
