#pragma once
#include <stdint.h>
typedef unsigned TickType_t;
#define pdTRUE 1
#define pdPASS 1
#define portTICK_PERIOD_MS 10
#define pdMS_TO_TICKS(x) ((TickType_t)(x)/10U+1U)
