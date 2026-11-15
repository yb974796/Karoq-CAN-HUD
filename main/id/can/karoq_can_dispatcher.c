#include "karoq_can_dispatcher.h"
#include "karoq_oem_nav_decoder.h" /* KAROQ_OEM_NAV_V114 */
#include "karoq_ui_acc_set.h"
#include "karoq_sd_logger.h"
#include "karoq_ts_vza_decoder.h"
#include "esp_timer.h"

esp_err_t karoq_can_dispatcher_receive(twai_message_t *msg, TickType_t wait_ticks)
{
    /* This wrapper must be used by the existing TWAI receive task; it does not
     * start a new RX task or compete to read the CAN controller RX queue.
     * The SD recorder remains the original passive twai_receive() owner.
     */
    esp_err_t result = karoq_sd_logger_receive(msg, wait_ticks);
    if (result == ESP_OK && msg && !msg->rtr && msg->data_length_code == 8U) {
        karoq_ui_acc_set_receive(msg->identifier,msg->extd,msg->rtr,msg->data,
               msg->data_length_code,(uint32_t)(esp_timer_get_time()/1000ULL));
    }
    if (result == ESP_OK && msg && msg->extd && !msg->rtr &&
        msg->data_length_code <= 8U) {
        karoq_ts_vza_receive(msg->identifier,true,false,msg->data,
            msg->data_length_code,(uint32_t)(esp_timer_get_time()/1000ULL));
        karoq_oem_nav_receive(msg->identifier,true,false,msg->data,
            msg->data_length_code,(uint32_t)(esp_timer_get_time()/1000ULL));
    }
    return result;
}
