#include "karoq_sd_logger.h"
#include "karoq_can_dispatcher.h"
// V0.7: low-overhead read-only CAN sniffer. Replaces old V0.6 .c/.h.
#include "karoq_can_sniffer.h"
#include "karoq_mqb_openpilot_probe.h"
#include "karoq_mqb_assist_runtime.h"
#include "karoq_can_speed.h"
#include "karoq_can_acc_probe.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAN_RX_GPIO               GPIO_NUM_2
#define CAN_TX_UNWIRED_GPIO       GPIO_NUM_1
#define CAN_BITRATE_KBPS          500  // Verified by valid real-vehicle RX frames
#define CAN_RX_QUEUE_LEN          1024
#define CAN_ID_TABLE_SIZE         2048 // power of two, open-addressed hash
#define CAN_STATUS_PERIOD_MS      5000
#define CAN_RX_TASK_PRIORITY      15
#define CAN_STATS_TASK_PRIORITY    3

static const char *TAG = "KAROQ_CAN";
static bool s_started = false;
static karoq_can_frame_cb_t s_callback = NULL;
static uint32_t s_seen_key[CAN_ID_TABLE_SIZE];
static uint8_t s_seen_occupied[CAN_ID_TABLE_SIZE];
static uint32_t s_unique = 0;
static uint32_t s_unique_overflow = 0;
static uint64_t s_total = 0;
static portMUX_TYPE s_counter_mux = portMUX_INITIALIZER_UNLOCKED;

void karoq_can_sniffer_set_callback(karoq_can_frame_cb_t callback)
{
    if (!s_started) s_callback = callback;
}

static inline void mark_seen(uint32_t id, bool extended)
{
    // Distinguish standard and extended frames even when identifier numerically equal.
    uint32_t key = id | (extended ? (1U << 29) : 0U);
    uint32_t at = (key * 2654435761U) & (CAN_ID_TABLE_SIZE - 1U);
    for (unsigned tries = 0; tries < CAN_ID_TABLE_SIZE; ++tries) {
        if (!s_seen_occupied[at]) {
            s_seen_occupied[at] = 1;
            s_seen_key[at] = key;
            portENTER_CRITICAL(&s_counter_mux);
            ++s_unique;
            portEXIT_CRITICAL(&s_counter_mux);
            return;
        }
        if (s_seen_key[at] == key) return;
        at = (at + 1U) & (CAN_ID_TABLE_SIZE - 1U);
    }
    // Not a frame-loss counter; only indicates distinct IDs beyond table capacity.
    portENTER_CRITICAL(&s_counter_mux);
    ++s_unique_overflow;
    portEXIT_CRITICAL(&s_counter_mux);
}

static void can_rx_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "RX active: GPIO2, 500 kbps, LISTEN_ONLY, TX wire disconnected");
    while (true) {
        twai_message_t msg = {0};
        esp_err_t rc = karoq_can_dispatcher_receive(&msg, pdMS_TO_TICKS(100));
        if (rc == ESP_OK) {
            karoq_can_frame_t frame = {0};
            frame.id = msg.identifier;
            frame.extended = (msg.extd != 0);
            frame.rtr = (msg.rtr != 0);
            frame.dlc = msg.data_length_code > 8 ? 8 : msg.data_length_code;
            frame.timestamp_ms = (uint64_t)esp_timer_get_time() / 1000ULL;
            if (!frame.rtr) memcpy(frame.data, msg.data, frame.dlc);
            // No per-frame logging here, especially not on first appearance of an ID.
            mark_seen(frame.id, frame.extended);
            karoq_speed_ingest(&frame);
            karoq_mqb_openpilot_probe_ingest(&msg);
            karoq_mqb_assist_runtime_ingest(&msg);
            karoq_acc_ingest(&frame);  // non-blocking; no logging in RX task
            if (s_callback) s_callback(&frame); // user callback must be non-blocking
            portENTER_CRITICAL(&s_counter_mux);
            ++s_total;
            portEXIT_CRITICAL(&s_counter_mux);
        } else if (rc != ESP_ERR_TIMEOUT) {
            // Driver may return INVALID_STATE if stopped; no busy-spin/repeated logging.
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

// Slow diagnostic task: prints ACC raw changes without stalling CAN RX.
static void can_acc_probe_task(void *arg)
{
    (void)arg;
    uint32_t last_seq = 0;
    bool last_valid = false;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(250));
        karoq_acc_snapshot_t s = karoq_acc_get();
        if (s.valid && (!last_valid || s.seq != last_seq)) {
            if (s.target_valid) {
                ESP_LOGI("KAROQ_ACC", "ACC_CHANGE seq=%" PRIu32
                    " DISP=%u PRIMARY=%u SET=%.1f km/h raw=%03X data=%02X %02X %02X %02X %02X %02X %02X %02X",
                    s.seq, (unsigned)s.status_display, (unsigned)s.status_primary,
                    (double)s.target_kmh, (unsigned)s.target_raw,
                    s.data[0],s.data[1],s.data[2],s.data[3],
                    s.data[4],s.data[5],s.data[6],s.data[7]);
            } else {
                ESP_LOGI("KAROQ_ACC", "ACC_CHANGE seq=%" PRIu32
                    " DISP=%u PRIMARY=%u SET=-- raw=%03X data=%02X %02X %02X %02X %02X %02X %02X %02X",
                    s.seq, (unsigned)s.status_display, (unsigned)s.status_primary,
                    (unsigned)s.target_raw,
                    s.data[0],s.data[1],s.data[2],s.data[3],
                    s.data[4],s.data[5],s.data[6],s.data[7]);
            }
        } else if (!s.valid && last_valid) {
            ESP_LOGW("KAROQ_ACC", "ACC_02 stale: no 0x30C frame in 750ms");
        }
        last_valid = s.valid;
        last_seq = s.seq;
    }
}

static void can_stats_task(void *arg)
{
    (void)arg;
    uint32_t prev_missed = 0, prev_overrun = 0, prev_bus_err = 0;
    uint64_t prev_total = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(CAN_STATUS_PERIOD_MS));
        twai_status_info_t st = {0};
        if (twai_get_status_info(&st) != ESP_OK) continue;
        uint64_t frames;
        portENTER_CRITICAL(&s_counter_mux);
        frames = s_total;
        portEXIT_CRITICAL(&s_counter_mux);
        uint32_t ids, id_over;
        portENTER_CRITICAL(&s_counter_mux);
        ids = s_unique;
        id_over = s_unique_overflow;
        portEXIT_CRITICAL(&s_counter_mux);
        karoq_speed_snapshot_t speed = karoq_speed_get();
        char speed_text[20];
        if (speed.valid) {
            int tenths = (int)(speed.kmh * 10.0f + 0.5f);
            snprintf(speed_text, sizeof(speed_text), "%d.%01d", tenths / 10, tenths % 10);
        } else {
            snprintf(speed_text, sizeof(speed_text), "--");
        }
        ESP_LOGI(TAG,
            "STATUS total=%" PRIu64 " rx_5s=%" PRIu64 " ids=%" PRIu32
            " id_table_over=%" PRIu32 " pending=%" PRIu32
            " lost=%" PRIu32 " (+%" PRIu32 ") overrun=%" PRIu32
            " (+%" PRIu32 ") bus_err=%" PRIu32 " (+%" PRIu32 ")"
            " SPEED=%s km/h src=%s age=%" PRIu32 "ms",
            frames, frames - prev_total, ids, id_over, st.msgs_to_rx,
            st.rx_missed_count, st.rx_missed_count - prev_missed,
            st.rx_overrun_count, st.rx_overrun_count - prev_overrun,
            st.bus_error_count, st.bus_error_count - prev_bus_err,
            speed_text, karoq_speed_source_name(speed.source), speed.age_ms);
        prev_total = frames;
        prev_missed = st.rx_missed_count;
        prev_overrun = st.rx_overrun_count;
        prev_bus_err = st.bus_error_count;
    }
}

esp_err_t karoq_can_sniffer_start(void)
{
    if (s_started) return ESP_ERR_INVALID_STATE;
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_UNWIRED_GPIO, CAN_RX_GPIO, TWAI_MODE_LISTEN_ONLY);
    g.tx_queue_len = 0;
    g.rx_queue_len = CAN_RX_QUEUE_LEN;
    g.alerts_enabled = TWAI_ALERT_NONE;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    esp_err_t rc = twai_driver_install(&g, &t, &f);
    if (rc != ESP_OK) return rc;
    memset(s_seen_occupied, 0, sizeof(s_seen_occupied));
    s_total = 0;
    s_unique = 0;
    s_unique_overflow = 0;
    rc = twai_start();
    if (rc != ESP_OK) {
        twai_driver_uninstall();
        return rc;
    }
    s_started = true;
    if (xTaskCreate(can_rx_task, "karoq_can_rx", 4096, NULL,
                    CAN_RX_TASK_PRIORITY, NULL) != pdPASS) {
        s_started = false;
        twai_stop(); twai_driver_uninstall();
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(can_stats_task, "karoq_can_stats", 4096, NULL,
                    CAN_STATS_TASK_PRIORITY, NULL) != pdPASS) {
        // RX has already started, keep it running rather than uninstalling the driver.
        ESP_LOGW(TAG, "Stats task creation failed; CAN RX remains active");
    }
    if (xTaskCreate(can_acc_probe_task, "karoq_acc_probe", 4096, NULL,
                    CAN_STATS_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGW(TAG, "ACC probe task creation failed; CAN RX remains active");
    }
    ESP_LOGI(TAG, "TWAI start OK: RX=GPIO2, TX=GPIO1(unwired), listen-only, %d kbps", CAN_BITRATE_KBPS);
    return ESP_OK;
}
