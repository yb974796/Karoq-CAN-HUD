#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "karoq_id_tracker.h"
static karoq_id_tracker_t t;

static void run_all(void)
{
    uint8_t a[8]={0x12,0x34,0,0,0,0,0,0};
    uint8_t b[8]={0x12,0x35,0,0,0,0,0,0};
    karoq_id_tracker_reset(&t);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,2,a)==KAROQ_ID_NEW_SESSION);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,2,a)==KAROQ_ID_REPEAT);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,2,b)==KAROQ_ID_CHANGED);
    assert(karoq_id_tracker_ingest(&t,0x0FD,true,false,2,b)==KAROQ_ID_NEW_SESSION);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,true,2,b)==KAROQ_ID_CHANGED);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,true,2,b)==KAROQ_ID_REPEAT);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,2,a)==KAROQ_ID_CHANGED);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,0,a)==KAROQ_ID_CHANGED);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,0,b)==KAROQ_ID_REPEAT);
    assert(t.distinct==2);
    karoq_id_tracker_reset(&t);
    assert(t.distinct==0 && t.full_events==0);
    assert(karoq_id_tracker_ingest(&t,0x0FD,false,false,2,a)==KAROQ_ID_NEW_SESSION);

    /* 1024 slots: collisions and capacity overflow must not corrupt the table. */
    karoq_id_tracker_reset(&t);
    for(uint32_t id=0;id<KAROQ_TRACKER_CAPACITY;id++) {
        uint8_t d[8]={(uint8_t)id};
        assert(karoq_id_tracker_ingest(&t,id,false,false,1,d)==KAROQ_ID_NEW_SESSION);
    }
    assert(t.distinct==KAROQ_TRACKER_CAPACITY);
    assert(karoq_id_tracker_ingest(&t,2000,false,false,1,a)==KAROQ_ID_TABLE_FULL);
    assert(t.full_events==1);
    assert(karoq_id_tracker_ingest(&t,1013,false,false,1,a)==KAROQ_ID_CHANGED);
}
int main(void)
{
    for(unsigned i=0;i<10;i++)run_all();
    puts("PASS: 10x tracker first-seen/repeat/changed/RTR/DLC/STD-EXT/full-table and reset");
    return 0;
}
