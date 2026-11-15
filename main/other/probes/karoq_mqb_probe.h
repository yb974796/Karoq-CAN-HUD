#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Diagnostic-only MQB DBC candidates. Network occurrence and payload
 * mapping are not verified on this Karoq Comfort CAN; NEVER use these to
 * set a live HUD assist_state without verified vehicle captures. */
typedef struct {
    uint32_t can_id;
    const char *name;
    const char *meaning;
} karoq_mqb_candidate_t;

const karoq_mqb_candidate_t *karoq_mqb_find_candidate(uint32_t can_id);
size_t karoq_mqb_candidate_count(void);
const karoq_mqb_candidate_t *karoq_mqb_candidate_at(size_t index);

/* Extracts PUBLIC DBC raw research fields only. NOT a state classifier. */
typedef struct {
    uint32_t can_id;
    bool recognized;
    bool raw_fields_valid;
    uint8_t tsk_status;      /* only 0x120: data[3] & 7 */
    bool limiter_selected;   /* only 0x120: data[3] bit7 */
    bool ldw_green;          /* only 0x397: data[7] bit6 */
    bool ldw_yellow;         /* only 0x397: data[7] bit5 */
    bool ldw_fault;          /* only 0x397: data[7] bit7 */
    uint8_t eps_hca_status;  /* only 0x09F: data[4] & 15 */
    uint8_t acc_display;    /* only 0x30C: data[7] bits5..7 */
} karoq_mqb_probe_t;

bool karoq_mqb_probe_std(uint32_t can_id, const uint8_t *data,
                         size_t dlc, karoq_mqb_probe_t *out);
