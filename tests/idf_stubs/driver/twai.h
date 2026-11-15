#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
typedef struct {uint32_t identifier; unsigned extd:1; unsigned rtr:1; uint8_t data_length_code; uint8_t data[8];} twai_message_t;
esp_err_t twai_receive(twai_message_t*,TickType_t);
