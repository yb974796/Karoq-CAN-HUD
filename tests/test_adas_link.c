#include "karoq_mqb_assist_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void bits(uint8_t b[8],unsigned start,unsigned n,unsigned value){
    for(unsigned i=0;i<n;i++){
        unsigned bit=start+i;
        uint8_t m=(uint8_t)(1u<<(bit%8));
        if(value&(1u<<i))b[bit/8]|=m; else b[bit/8]&=(uint8_t)~m;
    }
}
static void tsk(karoq_assist_engine_t *e,unsigned status,unsigned ctr,uint64_t at){
    uint8_t b[8]={0};bits(b,24,3,status);bits(b,8,4,ctr);
    karoq_assist_engine_ingest(e,0x120,false,false,8,b,at);
}
static void acc(karoq_assist_engine_t *e,unsigned target,unsigned gap,uint64_t at){
    uint8_t b[8]={0};bits(b,12,10,target);bits(b,37,3,gap);
    karoq_assist_engine_ingest(e,0x30C,false,false,8,b,at);
}
static void lane(karoq_assist_engine_t *e,uint64_t at){
    uint8_t l[8]={0},ep[8]={0},h[8]={0};
    bits(l,62,1,1);bits(ep,32,4,3);bits(h,32,4,2);
    karoq_assist_engine_ingest(e,0x397,false,false,8,l,at);
    karoq_assist_engine_ingest(e,0x09F,false,false,8,ep,at);
    karoq_assist_engine_ingest(e,0x126,false,false,8,h,at);
}
static void active(karoq_assist_engine_t*e,uint64_t at){
    acc(e,400,3,at);
    tsk(e,3,0,at);tsk(e,3,1,at+20);tsk(e,3,2,at+40);
}
static void valid(karoq_assist_engine_t*e) {karoq_assist_engine_init(e);}
static int runs=0;
#define CHECK(cond,name) do {assert(cond);printf("PASS %02d %s\n",++runs,name);} while(0)
int main(void){
    karoq_assist_engine_t e; karoq_assist_view_t v;
    valid(&e);v=karoq_assist_engine_evaluate(&e,1000);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN&&!v.acc_known,"missing TSK -> unknown");
    valid(&e);tsk(&e,2,0,1000);v=karoq_assist_engine_evaluate(&e,1000);
    CHECK(v.display==KAROQ_MQB_ASSIST_OFF&&v.acc_available,"TSK standby -> OFF grey");
    valid(&e);active(&e,1000);v=karoq_assist_engine_evaluate(&e,1050);
    CHECK(v.display==KAROQ_MQB_ASSIST_ACC_ONLY&&v.acc_active,"TSK3 + ACC valid three samples => ACC ONLY");
    valid(&e);acc(&e,1023,3,1000);tsk(&e,3,0,1000);tsk(&e,3,1,1020);tsk(&e,3,2,1040);
    v=karoq_assist_engine_evaluate(&e,1050);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"ACC special raw 0x3FF invalid");
    valid(&e);acc(&e,400,0,1000);tsk(&e,3,0,1000);tsk(&e,3,1,1020);tsk(&e,3,2,1040);
    v=karoq_assist_engine_evaluate(&e,1050);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"gap0 limiter -> not ACC green");
    valid(&e);active(&e,1000);v=karoq_assist_engine_evaluate(&e,1900);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"TSK stale -> unknown");
    valid(&e);active(&e,1000);tsk(&e,3,3,2300);tsk(&e,3,4,2320);tsk(&e,3,5,2340);
    v=karoq_assist_engine_evaluate(&e,2350);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"ACC setpoint stale -> unknown");
    valid(&e);acc(&e,400,3,1000);tsk(&e,6,0,1000);v=karoq_assist_engine_evaluate(&e,1000);
    CHECK(v.acc_fault && v.display==KAROQ_MQB_ASSIST_UNKNOWN,"TSK fault code -> unknown not active");
    valid(&e);lane(&e,1000);v=karoq_assist_engine_evaluate(&e,1000);
    CHECK(v.lane_candidate && v.display==KAROQ_MQB_ASSIST_UNKNOWN,"LDW/EPS/HCA live -> candidate only");
    valid(&e);active(&e,1000);lane(&e,1040);v=karoq_assist_engine_evaluate(&e,1060);
    CHECK(v.travel_candidate && v.display==KAROQ_MQB_ASSIST_ACC_ONLY,"ACC+lane candidate not Travel green");
    valid(&e);uint8_t b[8]={0};bits(b,24,3,3);
    karoq_assist_engine_ingest(&e,0x120,true,false,8,b,1000);
    karoq_assist_engine_ingest(&e,0x120,false,true,8,b,1000);
    karoq_assist_engine_ingest(&e,0x120,false,false,7,b,1000);
    v=karoq_assist_engine_evaluate(&e,1000);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"reject extended RTR short DLC");
    valid(&e);acc(&e,400,3,1000);tsk(&e,3,0,1000);tsk(&e,3,0,1020);tsk(&e,3,0,1040);
    v=karoq_assist_engine_evaluate(&e,1040);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"counter stuck -> no ACC green");
    valid(&e);acc(&e,900,3,1000);tsk(&e,3,0,1000);tsk(&e,3,1,1020);tsk(&e,3,2,1040);
    v=karoq_assist_engine_evaluate(&e,1050);
    CHECK(v.display==KAROQ_MQB_ASSIST_UNKNOWN,"implausible setpoint -> no green");
    puts("SUMMARY: 13/13 PASS");
    return 0;
}
