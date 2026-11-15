#include "karoq_ui_acc_set.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t s_set_kmh;
static uint32_t s_updated_ms;
static bool s_seen;
static bool s_has_set;
static karoq_acc_ui_mode_t s_mode;
static uint8_t s_disp;
static uint8_t s_primary;

void karoq_ui_acc_set_receive(uint32_t id, bool extended, bool rtr,
                              const uint8_t *data, uint8_t dlc, uint32_t now_ms)
{
    if (id != 0x30CU || extended || rtr || !data || dlc != 8U) return;
    /* ACC_02: SET bits 12..21, 0.32 km/h/count; 0x3FF unavailable.
     * Display bits 61..63, primary bits 22..23.
     * Never infer SET from road sign/pACC VZA: source separation. */
    const uint16_t raw = ((uint16_t)(data[1] >> 4) |
                         ((uint16_t)data[2] << 4)) & 0x03FFU;
    const uint8_t disp = (uint8_t)((data[7] >> 5) & 7U);
    const uint8_t primary = (uint8_t)((data[2] >> 6) & 3U);
    const bool set_ok = raw >= 1U && raw <= 781U;
    karoq_acc_ui_mode_t mode = KAROQ_ACC_UI_OFF;
    /* Conservative mapping grounded in V1.0.14 logs:
     * DISP=0  PRIMARY=0 => OFF
     * DISP=2/4 PRIMARY=0 with SET => STANDBY
     * DISP=3  PRIMARY=1 with SET => ACTIVE
     * Unverified combinations => OFF, no SET (no false green). */
    if (set_ok && primary == 1U && disp == 3U)
        mode = KAROQ_ACC_UI_ACTIVE;
    else if (set_ok && primary == 0U && (disp == 2U || disp == 4U))
        mode = KAROQ_ACC_UI_STANDBY;
    const bool has_set = mode != KAROQ_ACC_UI_OFF;
    const uint16_t kmh = has_set ? (uint16_t)((raw * 32U + 50U)/100U) : 0U;
    portENTER_CRITICAL(&s_mux);
    s_seen=true;
    s_mode=mode;
    s_has_set=has_set;
    s_set_kmh=kmh;
    s_updated_ms=now_ms;
    s_disp=disp;
    s_primary=primary;
    portEXIT_CRITICAL(&s_mux);
}

karoq_ui_acc_set_t karoq_ui_acc_set_snapshot(uint32_t now_ms)
{
    karoq_ui_acc_set_t snap={0};
    uint32_t stamp;
    bool seen,has_set;
    uint16_t speed;
    karoq_acc_ui_mode_t mode;
    uint8_t display,primary;
    portENTER_CRITICAL(&s_mux);
    stamp=s_updated_ms;
    seen=s_seen;
    has_set=s_has_set;
    speed=s_set_kmh;
    mode=s_mode;
    display=s_disp;
    primary=s_primary;
    portEXIT_CRITICAL(&s_mux);
    /* The UI may capture now_ms just BEFORE the RX task publishes a new
     * frame timestamp. For small negative skew, clamp to zero instead of
     * unsigned underflow (0xFFFFFFFF) which makes valid ACC blink OFF.
     * A real uint32_t clock wrap still uses normal modular subtraction. */
    const uint32_t ahead=stamp-now_ms;
    const uint32_t elapsed=(now_ms<stamp && ahead<=150U) ?
        0U : (uint32_t)(now_ms-stamp);
    snap.mode=KAROQ_ACC_UI_OFF;
    if (seen && elapsed<=1200U) {
        snap.signal_fresh=true;
        snap.age_ms=elapsed;
        snap.mode=mode;
        snap.valid=has_set;
        snap.set_kmh=has_set?speed:0U;
        snap.active=mode==KAROQ_ACC_UI_ACTIVE;
        snap.display_raw=display;
        snap.primary_raw=primary;
    }
    return snap;
}
