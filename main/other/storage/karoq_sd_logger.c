#include "karoq_sd_logger.h"
#include "karoq_id_tracker.h"
#include "karoq_record_button.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* 10-run audited V1.0.4. TWAI remains 500 kbps, RX ONLY; no CAN TX here. */
#define CAN_LOG_QUEUE_LENGTH 1024
#define CAN_LOG_FLUSH_MS 1000
#define CAN_LOG_DIR "/sdcard/CANLOG"
#define SD_MOUNT_PATH "/sdcard"
/* Waveshare ESP32-S3-Touch-LCD-3.49 V2 official SDMMC 1-bit pins */
#define SD_CLK GPIO_NUM_41
#define SD_CMD GPIO_NUM_39
#define SD_D0  GPIO_NUM_40

static const char *TAG="KAROQ_SD";
typedef struct { twai_message_t msg; uint64_t timestamp_ms; } can_sample_t;
typedef enum { CMD_TOGGLE=1 } record_cmd_t;
static QueueHandle_t s_samples;
static QueueHandle_t s_commands;
static atomic_bool s_active = ATOMIC_VAR_INIT(false);
/* Guards the interval from observing s_active through xQueueSend. */
static atomic_uint s_rx_inflight = ATOMIC_VAR_INIT(0);
static atomic_uint s_dropped = ATOMIC_VAR_INIT(0);
static atomic_uint s_queue_peak = ATOMIC_VAR_INIT(0);
static karoq_id_tracker_t s_tracker;
static sdmmc_card_t *s_card;
static FILE *s_all,*s_new,*s_changed;
static char s_name_all[96],s_name_new[96],s_name_changed[96],s_name_meta[96];
static uint64_t s_written, s_changed_count, s_new_count;
static uint32_t s_io_errors;

bool karoq_sd_logger_active(void) {return atomic_load_explicit(&s_active,memory_order_acquire);}
uint32_t karoq_sd_logger_drop_count(void) {return atomic_load_explicit(&s_dropped,memory_order_relaxed);}

static void record_queue_peak(void)
{
    uint32_t n=(uint32_t)uxQueueMessagesWaiting(s_samples);
    uint32_t seen=atomic_load_explicit(&s_queue_peak,memory_order_relaxed);
    while (n>seen && !atomic_compare_exchange_weak_explicit(&s_queue_peak,&seen,n,
                memory_order_relaxed,memory_order_relaxed)) { /* retry */ }
}

/* Only called within original TWAI RX task, which continues all existing decoding.
 * SD writes are NEVER done here; queue sends are non-blocking.
 */
esp_err_t karoq_sd_logger_receive(twai_message_t *message, TickType_t wait_ticks)
{
    esp_err_t e=twai_receive(message,wait_ticks);
    if(e!=ESP_OK || !s_samples)return e;
    if(!atomic_load_explicit(&s_active,memory_order_acquire))return e;

    atomic_fetch_add_explicit(&s_rx_inflight,1,memory_order_acq_rel);
    /* Double-check prevents a late enqueue AFTER writer completed STOP drainage. */
    if(atomic_load_explicit(&s_active,memory_order_acquire)) {
        can_sample_t sample={.msg=*message,.timestamp_ms=(uint64_t)esp_timer_get_time()/1000ULL};
        if(xQueueSend(s_samples,&sample,0)!=pdTRUE)
            atomic_fetch_add_explicit(&s_dropped,1,memory_order_relaxed);
        else record_queue_peak();
    }
    atomic_fetch_sub_explicit(&s_rx_inflight,1,memory_order_release);
    return e;
}

/* Never calculate START/STOP in button task: it may be stale while SD is mounting.
 * Writer processes ordered, idempotent toggle events one at a time.
 */
void karoq_sd_logger_toggle(void)
{
    if(!s_commands)return;
    const record_cmd_t cmd=CMD_TOGGLE;
    if(xQueueSend(s_commands,&cmd,0)!=pdTRUE)
        ESP_LOGW(TAG,"Button event queue full: extra tap ignored");
}

static bool close_one(FILE **p)
{
    bool ok=true;
    if(!*p)return true;
    if(fflush(*p)!=0)ok=false;
    if(fclose(*p)!=0)ok=false;
    *p=NULL;
    return ok;
}
static bool close_files(void)
{
    /* Don't short-circuit: close all three even after the first IO error. */
    bool a=close_one(&s_all);
    bool b=close_one(&s_new);
    bool c=close_one(&s_changed);
    return a && b && c;
}
static void unmount_card(void)
{
    if(s_card) {esp_vfs_fat_sdcard_unmount(SD_MOUNT_PATH,s_card);s_card=NULL;}
}
static bool mount_card(void)
{
    if(s_card)return true;
    sdmmc_host_t host=SDMMC_HOST_DEFAULT();
    host.max_freq_khz=SDMMC_FREQ_DEFAULT;
    sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width=1; slot.clk=SD_CLK;slot.cmd=SD_CMD;slot.d0=SD_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    esp_vfs_fat_mount_config_t cfg={
        .format_if_mount_failed=false, .max_files=6, .allocation_unit_size=16*1024,
    };
    esp_err_t e=esp_vfs_fat_sdmmc_mount(SD_MOUNT_PATH,&host,&slot,&cfg,&s_card);
    if(e!=ESP_OK) {
        s_card=NULL;
        ESP_LOGE(TAG,"SD mount failed (%s). Capture remains OFF",esp_err_to_name(e));
        return false;
    }
    if(mkdir(CAN_LOG_DIR,0775)!=0 && errno!=EEXIST) {
        ESP_LOGE(TAG,"Cannot create %s (errno=%d)",CAN_LOG_DIR,errno);
        unmount_card();return false;
    }
    return true;
}
static void make_paths(unsigned sequence)
{
    snprintf(s_name_all,sizeof s_name_all,CAN_LOG_DIR "/CAN_%04u.csv",sequence);
    snprintf(s_name_new,sizeof s_name_new,CAN_LOG_DIR "/CAN_%04u_NEW.csv",sequence);
    snprintf(s_name_changed,sizeof s_name_changed,CAN_LOG_DIR "/CAN_%04u_CHANGED.csv",sequence);
    snprintf(s_name_meta,sizeof s_name_meta,CAN_LOG_DIR "/CAN_%04u_META.txt",sequence);
}
static bool open_files(void)
{
    struct stat st;
    unsigned i;
    for(i=1;i<=9999;i++) {
        make_paths(i);
        /* Treat all stat errors except ENOENT as errors; never overwrite a file. */
        const char *paths[]={s_name_all,s_name_new,s_name_changed,s_name_meta};
        bool collision=false;
        for(unsigned j=0;j<4;j++) {
            if(stat(paths[j],&st)==0) {collision=true;break;}
            if(errno!=ENOENT) {
                ESP_LOGE(TAG,"Cannot inspect session file %s, errno=%d",paths[j],errno);
                return false;
            }
        }
        if(!collision)break;
    }
    if(i>9999) {ESP_LOGE(TAG,"No available CAN_XXXX session number");return false;}
    s_all=fopen(s_name_all,"w");
    if(!s_all)goto open_error;
    s_new=fopen(s_name_new,"w");
    if(!s_new)goto open_error;
    s_changed=fopen(s_name_changed,"w");
    if(!s_changed)goto open_error;
    (void)setvbuf(s_all,NULL,_IOFBF,4096);
    (void)setvbuf(s_new,NULL,_IOFBF,2048);
    (void)setvbuf(s_changed,NULL,_IOFBF,2048);
    if(fputs("uptime_ms,frame_type,id,dlc,rtr,data,event\n",s_all)==EOF ||
       fputs("uptime_ms,frame_type,id,dlc,rtr,data,event\n",s_new)==EOF ||
       fputs("uptime_ms,frame_type,id,dlc,rtr,data,event\n",s_changed)==EOF ||
       fflush(s_all)!=0 || fflush(s_new)!=0 || fflush(s_changed)!=0) {
        ESP_LOGE(TAG,"CSV header write/flush failed");
        ++s_io_errors;
        (void)close_files();
        return false;
    }
    return true;
open_error:
    ESP_LOGE(TAG,"Cannot create session files (errno=%d)",errno);
    ++s_io_errors;
    (void)close_files();
    return false;
}
static bool write_row(FILE *file,const can_sample_t *s,const char *event)
{
    char payload[3*8+1];size_t used=0;
    const twai_message_t *m=&s->msg;
    unsigned dlc=m->data_length_code;
    if(dlc>8)dlc=8;
    if(m->rtr)dlc=0;
    for(unsigned j=0;j<dlc;j++) {
        int n=snprintf(payload+used,sizeof(payload)-used,"%02X",m->data[j]);
        if(n<0)return false;
        used+=(size_t)n;
        if(j+1<dlc)payload[used++]=' ';
    }
    payload[used]=0;
    int n=fprintf(file,"%" PRIu64 ",%s,0x%0*" PRIX32 ",%u,%u,%s,%s\n",
        s->timestamp_ms,m->extd?"EXT29":"STD11",m->extd?8:3,
        (uint32_t)m->identifier,(unsigned)m->data_length_code,
        (unsigned)m->rtr,payload,event);
    return n>=0 && !ferror(file);
}
static bool write_frame(const can_sample_t *s)
{
    const twai_message_t *m=&s->msg;
    karoq_id_event_t e=karoq_id_tracker_ingest(&s_tracker,m->identifier,
        m->extd!=0,m->rtr!=0,m->data_length_code,m->data);
    const char *category=e==KAROQ_ID_NEW_SESSION?"NEW_ID":
        e==KAROQ_ID_CHANGED?"DATA_CHANGED":
        e==KAROQ_ID_TABLE_FULL?"TABLE_FULL":"REPEAT";
    if(!write_row(s_all,s,category))return false;
    if(e==KAROQ_ID_NEW_SESSION) {
        if(!write_row(s_new,s,category))return false;
        ++s_new_count;
    } else if(e==KAROQ_ID_CHANGED) {
        if(!write_row(s_changed,s,category))return false;
        ++s_changed_count;
    }
    ++s_written;
    return true;
}

static void write_metadata(bool sd_error)
{
    if(!s_card)return;
    FILE *f=fopen(s_name_meta,"w");
    if(!f) {++s_io_errors;ESP_LOGW(TAG,"Cannot create session metadata");return;}
    int n=fprintf(f,
        "firmware=V1.0.14.6-R1\nui=CAN_SNIFFER_V0.6\n"
        "timestamp=uptime_ms_from_boot\nCAN_mode=LISTEN_ONLY_500kbps\n"
        "data_file=%s\nnew_file=%s\nchanged_file=%s\n"
        "frames_written=%" PRIu64 "\nnew_id_count=%" PRIu64 "\n"
        "changed_frames=%" PRIu64 "\nunique_ids=%" PRIu32 "\n"
        "tracker_full_events=%" PRIu32 "\n"
        "queue_dropped=%" PRIu32 "\nqueue_peak=%" PRIu32 "\n"
        "sd_error=%u\n",
        s_name_all,s_name_new,s_name_changed,s_written,s_new_count,
        s_changed_count,s_tracker.distinct,s_tracker.full_events,
        (uint32_t)atomic_load(&s_dropped),(uint32_t)atomic_load(&s_queue_peak),(unsigned)sd_error);
    bool flush_ok=(fflush(f)==0);
    bool close_ok=(fclose(f)==0);
    if(n<0 || !flush_ok || !close_ok) {
        ++s_io_errors; ESP_LOGW(TAG,"Metadata write/close failed");
    }
}

static void halt_session(bool sd_error)
{
    /* First close producer gate. Producers which previously saw active=true
     * must finish enqueuing BEFORE the writer drains its final backlog. */
    atomic_store_explicit(&s_active,false,memory_order_release);
    while(atomic_load_explicit(&s_rx_inflight,memory_order_acquire)!=0)
        vTaskDelay(1);
    can_sample_t pending;
    if(!sd_error) {
        while(xQueueReceive(s_samples,&pending,0)==pdTRUE) {
            if(!write_frame(&pending)) {sd_error=true;++s_io_errors;break;}
        }
    }
    /* Count items lost after an SD failure; never carry over stale rows. */
    while(xQueueReceive(s_samples,&pending,0)==pdTRUE)
        atomic_fetch_add_explicit(&s_dropped,1,memory_order_relaxed);
    if(!close_files()) {sd_error=true;++s_io_errors;}
    write_metadata(sd_error);
    unmount_card();
    ESP_LOGI(TAG,"STOP frames=%" PRIu64 " unique=%" PRIu32 " new=%" PRIu64
        " changed=%" PRIu64 " queue_peak=%" PRIu32 " queue_dropped=%" PRIu32
        " tracker_full=%" PRIu32 " io_errors=%" PRIu32 " media_error=%u",
        s_written,s_tracker.distinct,s_new_count,s_changed_count,
        (uint32_t)atomic_load(&s_queue_peak),(uint32_t)atomic_load(&s_dropped),s_tracker.full_events,
        s_io_errors,(unsigned)sd_error);
    if(sd_error)ESP_LOGE(TAG,"SD write/media problem: verify CSV tail and META; do not treat recording as lossless");
}
static bool flush_all(void)
{
    return fflush(s_all)==0 && fflush(s_new)==0 && fflush(s_changed)==0;
}
static void logger_task(void *arg)
{
    (void)arg;
    can_sample_t sample;
    record_cmd_t command;
    uint64_t last_flush=0,last_status=0;
    for(;;) {
        if(xQueueReceive(s_commands,&command,0)==pdTRUE && command==CMD_TOGGLE) {
            if(karoq_sd_logger_active()) {
                halt_session(false);
            } else {
                s_io_errors=0;
                if(mount_card() && open_files()) {
                    karoq_id_tracker_reset(&s_tracker);
                    s_written=s_changed_count=s_new_count=0;
                    atomic_store(&s_dropped,0);
                    atomic_store(&s_queue_peak,0);
                    /* Previous session fully stopped before reset. */
                    xQueueReset(s_samples);
                    last_flush=last_status=(uint64_t)esp_timer_get_time()/1000ULL;
                    atomic_store_explicit(&s_active,true,memory_order_release);
                    ESP_LOGI(TAG,"REC START: %s  |  NEW IDs: %s",s_name_all,s_name_new);
                } else {
                    if(!close_files())++s_io_errors;
                    unmount_card();
                    ESP_LOGW(TAG,"REC OFF: insert FAT32 SD card and press BOOT again");
                }
            }
        }
        if(!karoq_sd_logger_active()) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if(xQueueReceive(s_samples,&sample,pdMS_TO_TICKS(10))==pdTRUE) {
            if(!write_frame(&sample)) {++s_io_errors;halt_session(true);continue;}
        }
        uint64_t now=(uint64_t)esp_timer_get_time()/1000ULL;
        if(now-last_flush>=CAN_LOG_FLUSH_MS) {
            if(!flush_all()) {++s_io_errors;halt_session(true);continue;}
            last_flush=now;
        }
        if(now-last_status>=5000) {
            ESP_LOGI(TAG,"REC frames=%" PRIu64 " unique=%" PRIu32
                " changed=%" PRIu64 " queue=%u peak=%" PRIu32
                " dropped=%" PRIu32 " io_errors=%" PRIu32,
                s_written,s_tracker.distinct,s_changed_count,
                (unsigned)uxQueueMessagesWaiting(s_samples),
                (uint32_t)atomic_load(&s_queue_peak),(uint32_t)atomic_load(&s_dropped),s_io_errors);
            last_status=now;
        }
    }
}

esp_err_t karoq_sd_logger_init(void)
{
    if(s_samples)return ESP_OK;
    s_samples=xQueueCreate(CAN_LOG_QUEUE_LENGTH,sizeof(can_sample_t));
    s_commands=xQueueCreate(8,sizeof(record_cmd_t));
    if(!s_samples || !s_commands) {
        if(s_samples)vQueueDelete(s_samples);
        if(s_commands)vQueueDelete(s_commands);
        s_samples=s_commands=NULL;
        return ESP_ERR_NO_MEM;
    }
    if(xTaskCreate(logger_task,"karoq_sd_writer",8192,NULL,4,NULL)!=pdPASS) {
        vQueueDelete(s_samples);vQueueDelete(s_commands);
        s_samples=s_commands=NULL;
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err=karoq_record_button_init();
    if(err!=ESP_OK)ESP_LOGE(TAG,"BOOT button init failed (%s); CAN continues",esp_err_to_name(err));
    ESP_LOGI(TAG,"V1.0.10 idle; BOOT tap toggles SD. 1-bit SDMMC CMD39 D0=40 CLK41");
    return err;
}
