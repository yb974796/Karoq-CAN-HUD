#pragma once
#include "esp_err.h"
#include "driver/twai.h"
#ifdef __cplusplus
extern "C" {
#endif
// Call before starting TWAI (initializes a read-only low priority stats task).
esp_err_t karoq_mqb_openpilot_probe_init(void);
// Called ONLY after successful TWAI RX. No logging or blocking in RX path.
void karoq_mqb_openpilot_probe_ingest(const twai_message_t *msg);
#ifdef __cplusplus
}
#endif
