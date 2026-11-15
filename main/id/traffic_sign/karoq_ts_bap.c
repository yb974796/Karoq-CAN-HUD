#include "karoq_ts_bap.h"
#include <string.h>

void karoq_ts_bap_reset(karoq_ts_bap_t *s)
{
    if (s) memset(s, 0, sizeof(*s));
}

bool karoq_ts_bap_feed(karoq_ts_bap_t *s, const uint8_t *data, uint8_t dlc,
                       uint32_t now_ms, karoq_ts_bap_message_t *out)
{
    if (!s || !data || !out || dlc == 0U || dlc > 8U) return false;
    uint8_t h = data[0];
    /* Only BAP transport channel 1 (0x9n / 0xDn) belongs to VZA Function 10.
     * Other 0x80/0xC0 BAP traffic is interleaved and MUST NOT break this stream.
     */
    if (h == 0x90U) {
        s->active = false;
        if (dlc != 8U || data[1] != KAROQ_TS_BAP_APP_LEN) {
            s->bad_length++;
            return false;
        }
        /* The transport length is 18 application bytes + 2-byte BAP header. */
        memcpy(s->bytes, data + 2, 6U);
        s->count = 6U;
        s->next_seq = 0U;
        s->started_ms = now_ms;
        s->last_ms = now_ms;
        s->active = true;
        return false;
    }
    if (h != 0xD0U && h != 0xD1U) return false;
    if (!s->active) return false;
    if ((uint32_t)(now_ms - s->last_ms) > KAROQ_TS_BAP_TIMEOUT_MS) {
        s->active = false;
        s->timeout++;
        return false;
    }
    if (h != (uint8_t)(0xD0U + s->next_seq)) {
        s->active = false;
        s->bad_sequence++;
        return false;
    }
    if (dlc != 8U || (size_t)s->count + 7U > KAROQ_TS_BAP_WIRE_LEN) {
        s->active = false;
        s->bad_length++;
        return false;
    }
    memcpy(s->bytes + s->count, data + 1, 7U);
    s->count = (uint8_t)(s->count + 7U);
    s->next_seq++;
    s->last_ms = now_ms;
    if (s->count != KAROQ_TS_BAP_WIRE_LEN) return false;
    s->active = false;
    /* One complete, validated transport, but NOT necessarily a valid sign. */
    memset(out, 0, sizeof(*out));
    out->opcode = s->bytes[0];
    out->function = s->bytes[1];
    out->payload_len = KAROQ_TS_BAP_APP_LEN;
    memcpy(out->payload, s->bytes + 2, KAROQ_TS_BAP_APP_LEN);
    s->completed++;
    return true;
}
