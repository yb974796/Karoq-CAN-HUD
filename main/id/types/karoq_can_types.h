#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    bool extended;
    bool rtr;
    uint64_t timestamp_ms;
} karoq_can_frame_t;
