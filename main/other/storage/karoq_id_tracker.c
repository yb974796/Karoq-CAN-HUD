#include "karoq_id_tracker.h"
#include <string.h>
void karoq_id_tracker_reset(karoq_id_tracker_t *t) { memset(t, 0, sizeof(*t)); }

karoq_id_event_t karoq_id_tracker_ingest(karoq_id_tracker_t *t, uint32_t id,
        bool extended, bool remote, uint8_t dlc, const uint8_t data[8])
{
    if (dlc > 8) dlc = 8;
    const uint32_t h = (id * 2654435761U) ^ (extended ? 0xA5A5A5A5U : 0U);
    const unsigned start = (unsigned)(h & (KAROQ_TRACKER_CAPACITY - 1));
    for (unsigned i=0; i<KAROQ_TRACKER_CAPACITY; ++i) {
        karoq_id_entry_t *e = &t->entries[(start+i) & (KAROQ_TRACKER_CAPACITY-1)];
        if (!e->occupied) {
            e->occupied=true; e->id=id; e->extended=extended; e->remote=remote;
            e->dlc=dlc;
            if (!remote && dlc) memcpy(e->data,data,dlc);
            t->distinct++;
            return KAROQ_ID_NEW_SESSION;
        }
        if (e->id != id || e->extended != extended) continue;
        if (e->dlc == dlc && e->remote == remote &&
            (remote || dlc == 0 || memcmp(e->data,data,dlc) == 0)) return KAROQ_ID_REPEAT;
        e->dlc=dlc; e->remote=remote;
        if (!remote && dlc) memcpy(e->data,data,dlc);
        return KAROQ_ID_CHANGED;
    }
    t->full_events++;
    return KAROQ_ID_TABLE_FULL;
}
