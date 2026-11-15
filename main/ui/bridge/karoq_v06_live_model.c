#include "karoq_v06_live_model.h"
#include "karoq_can_speed.h"
#include "karoq_mqb_assist_runtime.h"
#include <math.h>

karoq_v06_live_model_t karoq_v06_live_model_read(void)
{
    karoq_v06_live_model_t out = {0};
    karoq_speed_snapshot_t speed = karoq_speed_get();
    karoq_assist_view_t assist = karoq_mqb_assist_runtime_get();
    /* The original V0.6 UI represents only integer 0..299; never clip to 299. */
    if (speed.valid && isfinite(speed.kmh) && speed.kmh >= 0.0f && speed.kmh < 299.5f) {
        out.speed_kph = (uint16_t)(speed.kmh + 0.5f);
        out.speed_valid = out.speed_kph <= 299U;
    }
    out.tsk_status = assist.tsk_status;
    out.acc_known = assist.acc_known;
    out.lane_candidate = assist.lane_candidate;
    out.travel_candidate = assist.travel_candidate;
    /* Independent observed conditions. HCA=5 comes from EPS, not 0x397 LED.
       TRAVEL sprite means only "ACC and Lane active" on this HUD, not
       confirmation of OEM Travel Assist mode or safety availability. */
    if (assist.acc_active && assist.lane_active) {
        out.assist = KAROQ_LINK_TRAVEL;
    } else if (assist.lane_active) {
        out.assist = KAROQ_LINK_LANE;
    } else if (assist.acc_active) {
        out.assist = KAROQ_LINK_ACC_ONLY;
    } else if (assist.acc_known || assist.lane_known) {
        out.assist = KAROQ_LINK_OFF;
    } else {
        out.assist = KAROQ_LINK_UNKNOWN;
    }
    return out;
}
