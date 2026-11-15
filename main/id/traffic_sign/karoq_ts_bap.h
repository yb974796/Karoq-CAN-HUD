#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Restricted VZA BAP reassembler. Input: CAN EXT29 0x17332110 only.
 * BAP transport: channel 1, first frame 90 12 followed by D0 / D1.
 * 0x12 = application BAP payload length (18), plus two-byte BAP header.
 * The two fragment channels are multiplexed on the same CAN ID.
 * The caller owns the state; one CAN-RX task calls feed(), no locks needed.
 */
#define KAROQ_TS_BAP_CAN_ID UINT32_C(0x17332110)
#define KAROQ_TS_BAP_TIMEOUT_MS UINT32_C(500)
#define KAROQ_TS_BAP_APP_LEN 18U
#define KAROQ_TS_BAP_WIRE_LEN (KAROQ_TS_BAP_APP_LEN + 2U)

typedef struct {
    bool active;
    uint8_t next_seq;
    uint8_t count;
    uint8_t bytes[32];
    uint32_t started_ms;
    uint32_t last_ms;
    uint32_t completed;
    uint32_t bad_sequence;
    uint32_t bad_length;
    uint32_t timeout;
} karoq_ts_bap_t;

typedef struct {
    uint8_t opcode; /* 0x38=HeartbeatStatus, 0x48=Status, raw BAP high nibble */
    uint8_t function; /* second BAP header byte, observed 0x50 */
    uint8_t payload_len;
    uint8_t payload[18];
} karoq_ts_bap_message_t;

void karoq_ts_bap_reset(karoq_ts_bap_t *s);
bool karoq_ts_bap_feed(karoq_ts_bap_t *s, const uint8_t *data, uint8_t dlc,
                       uint32_t now_ms, karoq_ts_bap_message_t *out);
