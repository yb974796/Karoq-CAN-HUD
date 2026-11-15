#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "karoq_nav_service.h"

/* From trusted external MHI2/MST2 RGI hook AFTER iAP2 normalization.
 * This does not intercept iAP2, change head-unit software, or start transport.
 */
bool karoq_nav_carplay_maneuver(karoq_nav_maneuver_t maneuver,
                                uint32_t remaining_m, uint32_t local_now_ms,
                                uint32_t ttl_ms);
void karoq_nav_carplay_disconnect(void);
