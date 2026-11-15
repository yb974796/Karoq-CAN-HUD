#pragma once
#include <stdbool.h>
#include <stdint.h>

/* ACC_02 0x30C, display-only. Conflicting combinations fail closed. */
typedef enum {
    KAROQ_ACC_UI_OFF = 0,
    KAROQ_ACC_UI_STANDBY = 1,
    KAROQ_ACC_UI_ACTIVE = 2
} karoq_acc_ui_mode_t;

typedef struct {
    bool valid;            /* true only with a meaningful SET value */
    bool active;           /* compatibility field */
    bool signal_fresh;     /* 0x30C received within 1200ms */
    uint16_t set_kmh;
    uint32_t age_ms;
    karoq_acc_ui_mode_t mode;
    uint8_t display_raw;
    uint8_t primary_raw;
} karoq_ui_acc_set_t;

void karoq_ui_acc_set_receive(uint32_t id, bool extended, bool rtr,
                              const uint8_t *data, uint8_t dlc, uint32_t now_ms);
karoq_ui_acc_set_t karoq_ui_acc_set_snapshot(uint32_t now_ms);
