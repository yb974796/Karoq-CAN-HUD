#include "karoq_mqb_assist_runtime.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <string.h>

static portMUX_TYPE s_lock=portMUX_INITIALIZER_UNLOCKED;
static karoq_assist_engine_t s_engine;
static bool s_initialized=false;

esp_err_t karoq_mqb_assist_runtime_init(void) {
    portENTER_CRITICAL(&s_lock);
    if (s_initialized) {
        portEXIT_CRITICAL(&s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    karoq_assist_engine_init(&s_engine);
    s_initialized=true;
    portEXIT_CRITICAL(&s_lock);
    ESP_LOGI("KAROQ_ADAS","V1.0.10 ACC02 + independent EPS HCA=5 Lane; TX disabled");
    return ESP_OK;
}

void karoq_mqb_assist_runtime_ingest(const twai_message_t *frame) {
    if(!s_initialized || !frame || frame->extd || frame->rtr || frame->data_length_code!=8) return;
    // Only the 5 selected ID definitions reach the critical section.
    uint32_t id=frame->identifier;
    if (id!=0x120 && id!=0x30C && id!=0x397 && id!=0x09F && id!=0x126) return;
    uint64_t now=(uint64_t)esp_timer_get_time()/1000ULL;
    portENTER_CRITICAL(&s_lock);
    karoq_assist_engine_ingest(&s_engine,id,false,false,8,frame->data,now);
    portEXIT_CRITICAL(&s_lock);
}

karoq_assist_view_t karoq_mqb_assist_runtime_get(void) {
    karoq_assist_engine_t copy;
    portENTER_CRITICAL(&s_lock);
    memcpy(&copy,&s_engine,sizeof(copy));
    portEXIT_CRITICAL(&s_lock);
    return karoq_assist_engine_evaluate(&copy,(uint64_t)esp_timer_get_time()/1000ULL);
}
