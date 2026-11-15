#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "karoq_nav_service.h"

/* 16-byte future bridge packet, network-byte-order fields:
 * 0..1  "KN", 2 version=1, 3 kind (1 update, 2 clear)
 * 4 src (1 RGI, 2 HU, 3 external), 5 normalized maneuver
 * 6..9 distance meters, 10..11 TTL milliseconds
 * 12..13 per-source sequence, 14..15 CRC16/CCITT-FALSE over bytes 0..13.
 * CRC detects accidental corruption; it is NOT authentication.
 * No UART, Wi-Fi, TCP or iAP2 transport is initialized by this module.
 * Call receive from ONE trusted provider-owner task only.
 */
#define KAROQ_NAV_WIRE_LEN 16U
uint16_t karoq_nav_wire_crc16(const uint8_t *bytes, size_t len);
bool karoq_nav_wire_receive(const uint8_t *packet, size_t len, uint32_t local_now_ms);
void karoq_nav_wire_disconnect(karoq_nav_source_t source);
