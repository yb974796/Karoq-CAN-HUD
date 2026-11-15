#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Wire reader, listen-only. It never sends to CAN or controls OEM systems. */
void karoq_oem_nav_receive(uint32_t can_id, bool extended, bool rtr,
                           const uint8_t *data, uint8_t dlc, uint32_t now_ms);
/* Called from existing V0.6 layout's 100ms LVGL tick; no new task/timer. */
void karoq_oem_nav_tick(uint32_t now_ms);
