#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Source: commaai/opendbc opendbc/dbc/vw_mqb.dbc, SHA
// bfa397188ad2bbea3bd0e766856a73e18054b4ff (GitHub blob SHA).
// This is a select subset of RECEIVE-ONLY signal metadata, not an actuation module.
// MQB / PQ / MEB and camera/radar/comfort networks are NOT interchangeable.
#define KAROQ_MQB_MAX_FIELDS 6

typedef struct {
    const char *key; // opendbc DBC signal name
    uint8_t start_bit;
    uint8_t bit_length;
    float scale;
    float offset;
} karoq_mqb_field_def_t;

typedef struct {
    uint32_t can_id;
    const char *message;
    const char *network_hint; // research only, never a confirmed physical bus
    uint8_t dlc;
    uint8_t num_fields;
    const karoq_mqb_field_def_t *fields;
} karoq_mqb_msg_def_t;

typedef struct {
    const karoq_mqb_msg_def_t *def;
    uint32_t raw[KAROQ_MQB_MAX_FIELDS];
    float physical[KAROQ_MQB_MAX_FIELDS];
} karoq_mqb_decoded_t;

// Generic Intel/LSB (@1+) extractor used by reviewed MQB subset.
bool karoq_mqb_extract_le(const uint8_t data[8], unsigned start_bit,
                           unsigned bit_length, uint32_t *out);
const karoq_mqb_msg_def_t *karoq_mqb_find(uint32_t can_id);
size_t karoq_mqb_count(void);
const karoq_mqb_msg_def_t *karoq_mqb_by_index(size_t i);
// FALSE for unknown, extended, RTR, truncated or malformed frame. No writes to CAN.
bool karoq_mqb_decode(uint32_t can_id, bool extended, bool rtr, uint8_t dlc,
                      const uint8_t data[8], karoq_mqb_decoded_t *out);
