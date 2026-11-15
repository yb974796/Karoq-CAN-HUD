#pragma once
#include "esp_err.h"
#include "karoq_can_types.h"
#ifdef __cplusplus
extern "C" {
#endif
// Callback is invoked in high-priority CAN RX task. MUST NOT call LVGL/log/block.
typedef void (*karoq_can_frame_cb_t)(const karoq_can_frame_t *frame);
void karoq_can_sniffer_set_callback(karoq_can_frame_cb_t callback);
esp_err_t karoq_can_sniffer_start(void);
#ifdef __cplusplus
}
#endif
