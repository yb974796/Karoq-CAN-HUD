/* Candidate ID catalog for VW MQB / comma.ai opendbc (vw_mqb.dbc).
 * Diagnostic support only. NOT CONFIRMED on 2019 Karoq Comfort CAN.
 * No transmit, no response, no frame injections, no live HUD decoding. */
#include "karoq_mqb_probe.h"
#include <string.h>

static const karoq_mqb_candidate_t candidates[] = {
    {0x086, "LWI_01",     "steering angle"},
    {0x09F, "LH_EPS_03",  "EPS / HCA feedback"},
    {0x0B2, "ESP_19",     "wheel speeds"},
    {0x0FD, "ESP_21",     "vehicle speed candidate"},
    {0x117, "ACC_10",     "front collision/AEB"},
    {0x120, "TSK_06",     "cruise status candidate"},
    {0x122, "ACC_06",     "ACC longitudinal request"},
    {0x126, "HCA_01",     "lateral torque request"},
    {0x12B, "GRA_ACC_01", "cruise buttons"},
    {0x2A7, "ACC_13",     "ACC functional limits"},
    {0x2A9, "ACC_15",     "ACC collision/cluster alerts"},
    {0x30B, "Kombi_01",  "cluster speed candidate"},
    {0x30C, "ACC_02",    "ACC set speed/cluster status"},
    {0x30F, "SWA_01",    "blind-spot assistance"},
    {0x397, "LDW_02",    "lane departure/lane assist"}
};

size_t karoq_mqb_candidate_count(void)
{
    return sizeof(candidates) / sizeof(candidates[0]);
}

const karoq_mqb_candidate_t *karoq_mqb_candidate_at(size_t index)
{
    return (index < karoq_mqb_candidate_count()) ? &candidates[index] : NULL;
}

const karoq_mqb_candidate_t *karoq_mqb_find_candidate(uint32_t can_id)
{
    for (size_t i = 0; i < karoq_mqb_candidate_count(); i++) {
        if (candidates[i].can_id == can_id) return &candidates[i];
    }
    return NULL;
}

bool karoq_mqb_probe_std(uint32_t can_id, const uint8_t *data,
                         size_t dlc, karoq_mqb_probe_t *out)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    out->can_id = can_id;
    out->recognized = karoq_mqb_find_candidate(can_id) != NULL;
    if (!out->recognized || !data || dlc < 8 || dlc > 8 || can_id > 0x7FF) {
        return false;
    }
    switch (can_id) {
        case 0x120:
            out->tsk_status = data[3] & 0x07U;
            out->limiter_selected = (data[3] & 0x80U) != 0;
            break;
        case 0x397:
            out->ldw_green = (data[7] & 0x40U) != 0;
            out->ldw_yellow = (data[7] & 0x20U) != 0;
            out->ldw_fault = (data[7] & 0x80U) != 0;
            break;
        case 0x09F:
            out->eps_hca_status = data[4] & 0x0FU;
            break;
        case 0x30C:
            out->acc_display = (data[7] >> 5) & 0x07U;
            break;
        default:
            return false;
    }
    out->raw_fields_valid = true;
    return true;
}
