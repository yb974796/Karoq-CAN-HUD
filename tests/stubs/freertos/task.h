#pragma once
#include "freertos/FreeRTOS.h"
#include <stddef.h>
int xTaskCreate(void (*fn)(void*),const char *,uint32_t,void*,int,void*);
void vTaskDelay(uint32_t);
