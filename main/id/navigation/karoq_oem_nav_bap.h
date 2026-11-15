#pragma once
#include <stdbool.h>
#include <stdint.h>

/* OEM Navigation_SD: LSG 0x32, passive EXT29 CAN 0x17333210/11.
 * BAP transport is completely separate from existing VZA reassembly.
 */
#define KAROQ_OEM_NAV_BAP_APP_MAX 250U
#define KAROQ_OEM_NAV_BAP_TIMEOUT_MS 1500U

typedef struct {
    bool active;
    uint16_t expected;
    uint16_t count;
    uint8_t next_seq;
    uint32_t last_ms;
    uint8_t bytes[KAROQ_OEM_NAV_BAP_APP_MAX + 2U];
} karoq_oem_bap_stream_t;

typedef struct {
    karoq_oem_bap_stream_t stream[2][4];
    uint32_t completed;
    uint32_t bad_sequence;
    uint32_t bad_length;
    uint32_t expired;
} karoq_oem_bap_t;

typedef struct {
    uint8_t opcode;
    uint8_t lsg;
    uint8_t function_id;
    uint16_t payload_len;
    uint8_t payload[KAROQ_OEM_NAV_BAP_APP_MAX];
} karoq_oem_bap_message_t;

void karoq_oem_bap_init(karoq_oem_bap_t *ctx);
bool karoq_oem_bap_feed(karoq_oem_bap_t *ctx,uint32_t can_id,
                        const uint8_t *data,uint8_t dlc,uint32_t now_ms,
                        karoq_oem_bap_message_t *out);
