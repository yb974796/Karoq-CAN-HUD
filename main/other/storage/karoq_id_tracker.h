#ifndef KAROQ_ID_TRACKER_H
#define KAROQ_ID_TRACKER_H
#include <stdbool.h>
#include <stdint.h>
#define KAROQ_TRACKER_CAPACITY 1024U

typedef enum {
    KAROQ_ID_REPEAT = 0,
    KAROQ_ID_CHANGED = 1,
    KAROQ_ID_NEW_SESSION = 2,
    KAROQ_ID_TABLE_FULL = 3
} karoq_id_event_t;

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
    bool extended;
    bool remote;
    bool occupied;
} karoq_id_entry_t;

typedef struct {
    karoq_id_entry_t entries[KAROQ_TRACKER_CAPACITY];
    uint32_t distinct;
    uint32_t full_events;
} karoq_id_tracker_t;

void karoq_id_tracker_reset(karoq_id_tracker_t *t);
karoq_id_event_t karoq_id_tracker_ingest(karoq_id_tracker_t *t, uint32_t id,
        bool extended, bool remote, uint8_t dlc, const uint8_t data[8]);
#endif
