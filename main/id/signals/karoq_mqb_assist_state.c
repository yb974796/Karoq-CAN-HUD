#include "karoq_mqb_assist_state.h"
#include <limits.h>
#include <string.h>

#define TSK_TIMEOUT_MS 650ULL
#define TSK_CHAIN_GAP_MS 200ULL  // count only truly consecutive observations
#define ACC_TIMEOUT_MS 1200ULL
#define ACC02_FALLBACK_TIMEOUT_MS 350ULL
#define ACC02_CHAIN_GAP_MS 250ULL
#define ACC02_ACTIVE_STREAK 3U
#ifndef KAROQ_V109_ACC02_FALLBACK_ENABLE
#define KAROQ_V109_ACC02_FALLBACK_ENABLE 1
#endif

#define LANE_TIMEOUT_MS 1200ULL
#define EPS_ACTIVE_TIMEOUT_MS 1500ULL  // EPS observed at 2Hz, fail closed
#define EPS_ACTIVE_CHAIN_GAP_MS 1100ULL
#define EPS_ACTIVE_CONFIRM_FRAMES 2U


static bool fresh(const karoq_assist_observation_t *s, uint64_t now, uint64_t limit) {
    return s->seen && now >= s->updated_ms && (now - s->updated_ms) <= limit;
}
static uint32_t age(const karoq_assist_observation_t *s, uint64_t now) {
    if (!s->seen || now < s->updated_ms) return UINT32_MAX;
    uint64_t dt=now-s->updated_ms;
    return dt > UINT32_MAX ? UINT32_MAX : (uint32_t)dt;
}
void karoq_assist_engine_init(karoq_assist_engine_t *e) {
    if(e) memset(e,0,sizeof(*e));
}
void karoq_assist_engine_ingest(karoq_assist_engine_t *e,
        uint32_t id, bool extended, bool rtr, uint8_t dlc,
        const uint8_t data[8], uint64_t now_ms)
{
    if(!e || !data || extended || rtr || dlc != 8) return;
    uint32_t raw;
    karoq_assist_observation_t *s = NULL;
    switch(id) {
    case 0x120: // TSK_06: TSK_Status 24|3@1+, counter 8|4
        s=&e->tsk;
        if(!karoq_mqb_extract_le(data,24,3,&raw)) return;
        // Both state and 4-bit counter must progress in the SAME fresh
        // sequence. Never reuse pre-disconnect confidence after a long gap.
        uint8_t next_state=(uint8_t)raw;
        (void)karoq_mqb_extract_le(data,8,4,&raw);
        uint8_t next_counter=(uint8_t)raw;
        bool continuous=s->seen && now_ms >= s->updated_ms &&
            now_ms-s->updated_ms <= TSK_CHAIN_GAP_MS &&
            s->tsk_status==next_state &&
            (uint8_t)((s->tsk_counter+1U)&0x0FU)==next_counter;
        if(continuous) {
            if(s->tsk_consistent<255) s->tsk_consistent++;
            if(s->tsk_counter_consistent<255) s->tsk_counter_consistent++;
        } else {
            s->tsk_consistent=1;
            s->tsk_counter_consistent=1;
        }
        s->tsk_status=next_state;
        s->tsk_counter=next_counter;
        break;
    case 0x30C: { // ACC_02: primary+display+target corroborated, never DISP alone.
        s=&e->acc;
        const bool previous_good = s->seen && now_ms >= s->updated_ms &&
            (now_ms - s->updated_ms) <= ACC02_CHAIN_GAP_MS &&
            s->acc_prim == 1U && s->acc_display == 3U &&
            s->acc_target_raw >= 1U && s->acc_target_raw <= 781U &&
            s->acc_gap != 0U;
        (void)karoq_mqb_extract_le(data,12,10,&raw); s->acc_target_raw=(uint16_t)raw;
        (void)karoq_mqb_extract_le(data,22,2,&raw); s->acc_prim=(uint8_t)raw;
        (void)karoq_mqb_extract_le(data,37,3,&raw); s->acc_gap=(uint8_t)raw;
        (void)karoq_mqb_extract_le(data,61,3,&raw); s->acc_display=(uint8_t)raw;
        const bool now_good = s->acc_prim == 1U && s->acc_display == 3U &&
            s->acc_target_raw >= 1U && s->acc_target_raw <= 781U &&
            s->acc_gap != 0U;
        if (now_good) {
            if (previous_good) {
                if (s->acc02_good_streak < 255U) s->acc02_good_streak++;
            } else s->acc02_good_streak=1U;
        } else s->acc02_good_streak=0U;
        break;
    }
    case 0x397: // LDW_02: LED and warnings, NOT a steering engagement proof
        s=&e->ldw;
        (void)karoq_mqb_extract_le(data,62,1,&raw); s->ldw_green=(uint8_t)raw;
        (void)karoq_mqb_extract_le(data,56,1,&raw); s->ldw_left_warning=(uint8_t)raw;
        (void)karoq_mqb_extract_le(data,57,1,&raw); s->ldw_right_warning=(uint8_t)raw;
        break;
    case 0x09F: { // LH_EPS_03: actual EPS HCA status; independent of ACC/TSK
        s=&e->eps;
        const bool prior_active=s->seen && s->eps_hca_status==5U &&
            now_ms >= s->updated_ms &&
            (now_ms - s->updated_ms) <= EPS_ACTIVE_CHAIN_GAP_MS;
        (void)karoq_mqb_extract_le(data,32,4,&raw);
        s->eps_hca_status=(uint8_t)raw;
        if (s->eps_hca_status==5U) {
            if (prior_active) {
                if (s->eps_active_streak<255U) s->eps_active_streak++;
            } else s->eps_active_streak=1U;
        } else s->eps_active_streak=0U; // READY/FAULT/REJECTED cancels immediately
        break;
    }
    case 0x126:
        s=&e->hca;
        (void)karoq_mqb_extract_le(data,32,4,&raw); s->hca_status=(uint8_t)raw;
        break;
    default: return;
    }
    s->seen=true;
    s->updated_ms=now_ms;
}

karoq_assist_view_t karoq_assist_engine_evaluate(const karoq_assist_engine_t *e,uint64_t now_ms) {
    karoq_assist_view_t out={0};
    out.display=KAROQ_MQB_ASSIST_UNKNOWN;
    out.tsk_age_ms=out.acc_age_ms=UINT32_MAX;
    if(!e) return out;
    out.tsk_seen=e->tsk.seen;
    out.tsk_status=e->tsk.tsk_status;
    out.tsk_age_ms=age(&e->tsk,now_ms);
    out.acc_age_ms=age(&e->acc,now_ms);
    out.ldw_green=e->ldw.ldw_green;
    bool tsk=fresh(&e->tsk,now_ms,TSK_TIMEOUT_MS);
    bool acc=fresh(&e->acc,now_ms,ACC_TIMEOUT_MS);
    bool ldw=fresh(&e->ldw,now_ms,LANE_TIMEOUT_MS);
    bool eps=fresh(&e->eps,now_ms,LANE_TIMEOUT_MS);
    bool hca=fresh(&e->hca,now_ms,LANE_TIMEOUT_MS);
    // Pure EPS status is valid even when 0x120/0x126 are absent from HUD CAN.
    // OpenDBC EPS_HCA_Status: 3=READY, 5=ACTIVE. No vehicle control TX.
    out.lane_known=fresh(&e->eps,now_ms,EPS_ACTIVE_TIMEOUT_MS);
    out.lane_active=out.lane_known && e->eps.eps_hca_status==5U &&
        e->eps.eps_active_streak>=EPS_ACTIVE_CONFIRM_FRAMES;
    out.lane_data_live=ldw && eps && hca;
    // Indicator only: candidate raw evidence is never enough to assert LANE_ACTIVE.
    out.lane_candidate=out.lane_data_live && e->ldw.ldw_green &&
                       e->eps.eps_hca_status!=0 && e->hca.hca_status!=0;
    if (!tsk) {
        // TSK_06 was not observed in either CAN_0003/0004 capture.
        // Fallback ONLY when TSK has NEVER been seen, not when it goes stale.
        // Empirical 0x30C candidate: Prim=1, Display=3, valid target.
        // 3 consecutive frames + 350ms freshness; informational HUD ONLY.
        // No Lane/Travel green: LDW LED and EPS do not prove steering control.
#if KAROQ_V109_ACC02_FALLBACK_ENABLE
        if (!e->tsk.seen && fresh(&e->acc,now_ms,ACC02_FALLBACK_TIMEOUT_MS)) {
            out.acc_known=true;
            out.acc_available=true;
            out.display=KAROQ_MQB_ASSIST_OFF;
            if (e->acc.acc02_good_streak >= ACC02_ACTIVE_STREAK) {
                out.acc_active=true;
                out.display=KAROQ_MQB_ASSIST_ACC_ONLY;
            }
        }
#endif
        return out;
    }
    out.acc_known=true;
    out.acc_available=e->tsk.tsk_status>=2 && e->tsk.tsk_status<=5;
    out.acc_fault=e->tsk.tsk_status>=6;
    if(out.acc_fault){out.display=KAROQ_MQB_ASSIST_UNKNOWN;return out;}
    if(e->tsk.tsk_status<=2) {out.display=KAROQ_MQB_ASSIST_OFF;return out;}
    // MQB openpilot enabled 3..5; require 3 consistent TSK frames with incrementing
    // counter plus a live plausible ACC set-speed and non-limiter gap. Prevents
    // transient, ambiguous or single-message false green status.
    bool setpoint=acc && e->acc.acc_target_raw!=1023 &&
        (e->acc.acc_target_raw * 0.32f) <= 250.0f && e->acc.acc_gap!=0;
    if(e->tsk.tsk_consistent>=3 && e->tsk.tsk_counter_consistent>=3 && setpoint) {
        out.acc_active=true;
        out.display=KAROQ_MQB_ASSIST_ACC_ONLY;
    }
    // True Travel/Lane active requires vehicle-specific control-state validation;
    // NEVER infer solely from LDW LED or EPS/HCA nonzero status.
    out.travel_candidate=out.acc_active && out.lane_candidate;
    return out;
}
const char *karoq_assist_name(karoq_assist_display_t state) {
    switch(state){
        case KAROQ_MQB_ASSIST_OFF:return "OFF";
        case KAROQ_MQB_ASSIST_ACC_ONLY:return "ACC_ONLY";
        case KAROQ_MQB_ASSIST_LANE_ACTIVE:return "LANE_ACTIVE";
        case KAROQ_MQB_ASSIST_TRAVEL_ACTIVE:return "TRAVEL_ACTIVE";
        default:return "UNKNOWN";
    }
}
