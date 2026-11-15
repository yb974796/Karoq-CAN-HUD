#pragma once
#include "karoq_mqb_assist_state.h"
#include "driver/twai.h"
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t karoq_mqb_assist_runtime_init(void);
// Non-blocking, called once from the same TWAI RX task as the MQB probe.
void karoq_mqb_assist_runtime_ingest(const twai_message_t *frame);
karoq_assist_view_t karoq_mqb_assist_runtime_get(void);
#ifdef __cplusplus
}
#endif
