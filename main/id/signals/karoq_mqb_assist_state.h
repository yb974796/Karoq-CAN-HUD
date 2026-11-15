#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "karoq_mqb_opendbc.h"

// Display states must not be interpreted as instructions to control the vehicle.
typedef enum {
    KAROQ_MQB_ASSIST_UNKNOWN = 0,
    KAROQ_MQB_ASSIST_OFF = 1,
    KAROQ_MQB_ASSIST_LANE_ACTIVE = 2,
    KAROQ_MQB_ASSIST_TRAVEL_ACTIVE = 3,
    KAROQ_MQB_ASSIST_ACC_ONLY = 4
} karoq_assist_display_t;

typedef struct {
    bool seen;
    uint64_t updated_ms;
    uint8_t tsk_status;
    uint8_t tsk_consistent;     // count of adjacent same-state valid frames
    uint8_t tsk_counter;
    uint8_t tsk_counter_consistent;
    uint16_t acc_target_raw;
    uint8_t acc_gap;
    uint8_t acc_display;
    uint8_t acc_prim;               // ACC_Status_Prim_Anz 0x30C bit 22..23
    uint8_t acc02_good_streak;     // continuous 0x30C active-candidate frames
    uint8_t ldw_green;
    uint8_t ldw_left_warning;
    uint8_t ldw_right_warning;
    uint8_t eps_hca_status;
    uint8_t eps_active_streak;       // saturating 2-frame active confirmation
    uint8_t hca_status;
} karoq_assist_observation_t;

typedef struct {
    karoq_assist_observation_t tsk, acc, ldw, eps, hca;
} karoq_assist_engine_t;

typedef struct {
    karoq_assist_display_t display;
    bool acc_known;
    bool acc_active;
    bool acc_available;
    bool acc_fault;
    bool lane_data_live;
    bool lane_known;               // 0x09F received within 1500ms
    bool lane_active;              // EPS HCA=5 two consecutive fresh frames
    bool lane_candidate;        // NOT CONFIRMED LANE ACTIVE
    bool travel_candidate;      // NOT CONFIRMED TRAVEL ACTIVE
    bool tsk_seen;
    uint8_t tsk_status;
    uint8_t ldw_green;
    uint32_t tsk_age_ms;
    uint32_t acc_age_ms;
} karoq_assist_view_t;

// One caller writes to engine. Snapshot/evaluate from a copy under runtime lock.
void karoq_assist_engine_init(karoq_assist_engine_t *engine);
void karoq_assist_engine_ingest(karoq_assist_engine_t *engine,
                               uint32_t id, bool extended, bool rtr,
                               uint8_t dlc, const uint8_t data[8], uint64_t now_ms);
karoq_assist_view_t karoq_assist_engine_evaluate(const karoq_assist_engine_t *engine,
                                                uint64_t now_ms);
const char *karoq_assist_name(karoq_assist_display_t state);
