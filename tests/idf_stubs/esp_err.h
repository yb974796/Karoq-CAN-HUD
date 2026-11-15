#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NO_MEM 0x101
static inline const char *esp_err_to_name(esp_err_t x){(void)x;return "MOCK_ERR";}
