#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Pass EXT29 0x17332110 frames to this pure CAN receive handler.
 * Called from the existing RX task, whether or not SD recording is active.
 */
void karoq_ts_vza_receive(uint32_t can_id, bool extended, bool rtr,
                           const uint8_t *data, uint8_t dlc, uint32_t timestamp_ms);
void karoq_ts_vza_reset(void);
uint32_t karoq_ts_vza_completed_count(void);
uint32_t karoq_ts_vza_rejected_count(void);
