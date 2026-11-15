#pragma once
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

/* Transparent receive fanout; preserves SD capture + original RX semantics.
 * NEVER transmits to the vehicle bus.
 */
esp_err_t karoq_can_dispatcher_receive(twai_message_t *msg, TickType_t wait_ticks);
