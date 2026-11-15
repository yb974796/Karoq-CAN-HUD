#include "karoq_record_button.h"
#include "karoq_sd_logger.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stddef.h>

/* BOOT = ESP32-S3 strapping pin GPIO0; DO NOT use PWR: power can be cut off. */
#define BUTTON_GPIO GPIO_NUM_0
#define BUTTON_POLL_MS 10
#define BUTTON_DEBOUNCE_MS 40
#define BUTTON_MIN_PRESS_MS 50
#define BUTTON_MAX_PRESS_MS 1200
#define BUTTON_EVENT_COOLDOWN_MS 300
static const char *TAG = "KAROQ_BUTTON";

static void button_task(void *arg)
{
    (void)arg;
    /* If BOOT is held during boot, ignore until stable released for 100 ms. */
    unsigned released=0;
    while (released<10) {
        if (gpio_get_level(BUTTON_GPIO) == 1) released++;
        else released=0;
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
    int stable=1,last_raw=1;
    unsigned same=0;
    int64_t down_ms=0;
    int64_t last_accepted_ms=-BUTTON_EVENT_COOLDOWN_MS;
    for (;;) {
        int raw=gpio_get_level(BUTTON_GPIO);
        if (raw==last_raw) { if (same<10) ++same; }
        else { same=0; last_raw=raw; }
        if (same>=BUTTON_DEBOUNCE_MS/BUTTON_POLL_MS && stable!=raw) {
            stable=raw;
            int64_t now=(int64_t)xTaskGetTickCount()*portTICK_PERIOD_MS;
            if (stable==0) down_ms=now;
            else if (down_ms!=0) {
                int64_t held=now-down_ms;
                if (held>=BUTTON_MIN_PRESS_MS && held<=BUTTON_MAX_PRESS_MS &&
                    now-last_accepted_ms>=BUTTON_EVENT_COOLDOWN_MS) {
                    last_accepted_ms=now;
                    ESP_LOGI(TAG,"BOOT short tap (%lld ms): toggle SD CAN capture",(long long)held);
                    karoq_sd_logger_toggle();
                } else {
                    ESP_LOGW(TAG,"Ignore long/short BOOT action (%lld ms)",(long long)held);
                }
                down_ms=0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

esp_err_t karoq_record_button_init(void)
{
    gpio_config_t cfg={0};
    cfg.pin_bit_mask=1ULL<<BUTTON_GPIO;
    cfg.mode=GPIO_MODE_INPUT;
    cfg.pull_up_en=GPIO_PULLUP_ENABLE;
    cfg.pull_down_en=GPIO_PULLDOWN_DISABLE;
    cfg.intr_type=GPIO_INTR_DISABLE;
    esp_err_t e=gpio_config(&cfg);
    if(e!=ESP_OK)return e;
    if(xTaskCreate(button_task,"karoq_rec_btn",3072,NULL,4,NULL)!=pdPASS)
        return ESP_ERR_NO_MEM;
    ESP_LOGI(TAG,"BOOT(GPIO0) short tap to START/STOP SD logging; PWR untouched");
    return ESP_OK;
}
