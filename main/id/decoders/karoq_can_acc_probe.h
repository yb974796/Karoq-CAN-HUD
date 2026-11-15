#pragma once
#include "karoq_can_types.h"
#ifdef __cplusplus
extern "C" {
#endif
// 0x30C / ACC_02 raw monitor; state codes are NOT yet mapped to ACC on/off.
typedef struct {
    bool valid;
    uint8_t data[8];
    uint32_t seq;                 // increments on meaningful raw signal change
    uint32_t age_ms;
    uint8_t status_display;      // ACC_Status_Anzeige, DBC bit 61|3
    uint8_t status_primary;      // ACC_Status_Prim_Anz, DBC bit 22|2
    uint16_t target_raw;         // ACC_Wunschgeschw_02, DBC bit 12|10
    bool target_valid;           // 0x3FF treated as unavailable
    float target_kmh;            // raw * 0.32
} karoq_acc_snapshot_t;

bool karoq_acc_decode(const karoq_can_frame_t *frame, karoq_acc_snapshot_t *out);
void karoq_acc_ingest(const karoq_can_frame_t *frame);
karoq_acc_snapshot_t karoq_acc_get(void);
#ifdef __cplusplus
}
#endif
