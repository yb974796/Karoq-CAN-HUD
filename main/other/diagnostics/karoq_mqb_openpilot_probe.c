#include "karoq_mqb_openpilot_probe.h"
#include "karoq_mqb_opendbc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Deliberately read-only. This module NEVER alters CAN, ACC/Steering control or LVGL.
// Bus availability must be measured. No assumption that camera messages reach J533/HUD.
#define MAX_MQB_ID 16
#define REPORT_PERIOD_MS 5000
#define RX_STALE_MS 1000

static const char *TAG="KAROQ_OP_MQB";
static portMUX_TYPE s_lock=portMUX_INITIALIZER_UNLOCKED;
static bool s_initialized=false;
typedef struct {
    uint32_t packets;
    uint32_t changes;
    uint64_t last_ms;
    uint32_t signals[KAROQ_MQB_MAX_FIELDS];
    bool ever_seen;
} sample_state_t;
static sample_state_t s_state[MAX_MQB_ID];

void karoq_mqb_openpilot_probe_ingest(const twai_message_t *msg)
{
    if (!s_initialized || !msg || msg->extd || msg->rtr || msg->data_length_code<8) return;
    karoq_mqb_decoded_t decoded;
    if(!karoq_mqb_decode(msg->identifier,false,false,msg->data_length_code,msg->data,&decoded))return;
    size_t idx=0;
    for (;idx<karoq_mqb_count();++idx) {
        if(karoq_mqb_by_index(idx)==decoded.def)break;
    }
    if(idx>=MAX_MQB_ID)return;
    const uint64_t timestamp_ms=(uint64_t)esp_timer_get_time()/1000ULL;
    portENTER_CRITICAL(&s_lock);
    sample_state_t *p=&s_state[idx];
    bool changed=!p->ever_seen;
    for(unsigned i=0;i<decoded.def->num_fields;i++) {
        if(p->signals[i]!=decoded.raw[i]) changed=true;
        p->signals[i]=decoded.raw[i];
    }
    if(changed)p->changes++;
    p->ever_seen=true;
    p->packets++;
    p->last_ms=timestamp_ms;
    portEXIT_CRITICAL(&s_lock);
}

static void report_task(void *unused)
{
    (void)unused;
    uint32_t previously_logged[MAX_MQB_ID]={0};
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
        sample_state_t copy[MAX_MQB_ID];
        portENTER_CRITICAL(&s_lock);
        memcpy(copy,s_state,sizeof(copy));
        portEXIT_CRITICAL(&s_lock);
        const uint64_t now_ms=(uint64_t)esp_timer_get_time()/1000ULL;
        unsigned total_seen=0,live=0;
        for(size_t i=0;i<karoq_mqb_count() && i<MAX_MQB_ID;i++) {
            const karoq_mqb_msg_def_t *d=karoq_mqb_by_index(i);
            if(!copy[i].ever_seen)continue;
            ++total_seen;
            bool fresh=now_ms>=copy[i].last_ms &&
                       now_ms-copy[i].last_ms<=RX_STALE_MS;
            if(fresh)++live;
            if(copy[i].changes==previously_logged[i])continue;
            previously_logged[i]=copy[i].changes;
            // Rate limited: at most 1 diagnostic line per known ID per five seconds.
            ESP_LOGI(TAG,"ID=0x%03" PRIX32 " %s %s cnt=%" PRIu32 " changes=%" PRIu32
                " field0=%" PRIu32 " field1=%" PRIu32 " field2=%" PRIu32 " field3=%" PRIu32,
                d->can_id,d->message,fresh?"LIVE":"STALE",copy[i].packets,copy[i].changes,
                copy[i].signals[0],copy[i].signals[1],copy[i].signals[2],copy[i].signals[3]);
        }
        ESP_LOGI(TAG,"MQB known IDs seen=%u/%u live=%u; missing not proof of ECU absence (CAN bus topology)",
                 total_seen,(unsigned)karoq_mqb_count(),live);
    }
}

esp_err_t karoq_mqb_openpilot_probe_init(void)
{
    if(s_initialized)return ESP_ERR_INVALID_STATE;
    if(karoq_mqb_count()>MAX_MQB_ID)return ESP_ERR_INVALID_SIZE;
    memset(s_state,0,sizeof(s_state));
    s_initialized=true;
    if(xTaskCreate(report_task,"mqb_probe",4096,NULL,3,NULL)!=pdPASS) {
        s_initialized=false;
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG,"V1.0.9 MQB 12-ID receive-only probe started; HUD V0.6 unchanged");
    return ESP_OK;
}
