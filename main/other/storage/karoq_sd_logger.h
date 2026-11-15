#ifndef KAROQ_SD_LOGGER_H
#define KAROQ_SD_LOGGER_H
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "driver/twai.h"
#include <stdbool.h>
#include <stdint.h>

/* Created once before karoq_can_sniffer_start(). Does not mount/write SD until BOOT tap. */
esp_err_t karoq_sd_logger_init(void);
/* Drop-in wrapper: replaces twai_receive() inside the existing CAN RX task. */
esp_err_t karoq_sd_logger_receive(twai_message_t *message, TickType_t wait_ticks);
void karoq_sd_logger_toggle(void);
bool karoq_sd_logger_active(void);
uint32_t karoq_sd_logger_drop_count(void);
#endif
