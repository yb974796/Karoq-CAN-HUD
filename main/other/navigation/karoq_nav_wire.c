#include "karoq_nav_wire.h"
#include <stdint.h>

#define KIND_UPDATE 1U
#define KIND_CLEAR  2U

static bool s_seen[4];
static uint16_t s_seq[4];

uint16_t karoq_nav_wire_crc16(const uint8_t *bytes, size_t len)
{
    uint16_t crc = 0xFFFFU;
    if (!bytes && len) return 0;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)bytes[i] << 8;
        for (unsigned b = 0; b < 8; ++b)
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
    return crc;
}

static uint16_t be16(const uint8_t *s)
{
    return (uint16_t)(((uint16_t)s[0] << 8) | s[1]);
}
static uint32_t be32(const uint8_t *s)
{
    return ((uint32_t)s[0] << 24) | ((uint32_t)s[1] << 16) |
           ((uint32_t)s[2] << 8) | (uint32_t)s[3];
}

bool karoq_nav_wire_receive(const uint8_t *packet, size_t len, uint32_t local_now_ms)
{
    if (!packet || len != KAROQ_NAV_WIRE_LEN || packet[0] != 'K' ||
        packet[1] != 'N' || packet[2] != 1U ||
        packet[4] < 1U || packet[4] > 3U) return false;
    if (be16(&packet[14]) != karoq_nav_wire_crc16(packet, 14)) return false;
    if (packet[3] != KIND_UPDATE && packet[3] != KIND_CLEAR) return false;
    const unsigned src = packet[4];
    const uint16_t seq = be16(&packet[12]);
    if (s_seen[src]) {
        const uint16_t delta = (uint16_t)(seq - s_seq[src]);
        if (delta == 0U || delta >= 0x8000U) return false;
    }
    if (packet[3] == KIND_CLEAR) {
        /* Clear must have an unambiguous all-zero payload. */
        for (unsigned i = 5U; i <= 11U; ++i) if (packet[i] != 0U) return false;
        karoq_nav_service_invalidate((karoq_nav_source_t)src);
    } else {
        const karoq_nav_event_t e = {
            .source = (karoq_nav_source_t)src,
            .maneuver = (karoq_nav_maneuver_t)packet[5],
            .distance_m = be32(&packet[6]),
            .timestamp_ms = local_now_ms,
            .valid_ms = be16(&packet[10])
        };
        if (!karoq_nav_service_publish(&e)) return false;
    }
    s_seq[src] = seq;
    s_seen[src] = true;
    return true;
}

void karoq_nav_wire_disconnect(karoq_nav_source_t source)
{
    if (source < KAROQ_NAV_SRC_CARPLAY_RGI || source > KAROQ_NAV_SRC_EXTERNAL) return;
    s_seen[source] = false;
    karoq_nav_service_invalidate(source);
}
