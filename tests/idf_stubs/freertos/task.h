#pragma once
#include "freertos/FreeRTOS.h"
int xTaskCreate(void (*fn)(void*),const char*,unsigned,void*,int,void*);
void vTaskDelay(TickType_t);
TickType_t xTaskGetTickCount(void);
