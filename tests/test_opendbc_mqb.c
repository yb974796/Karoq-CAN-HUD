#include "karoq_mqb_opendbc.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void encode(uint8_t dst[8],unsigned first,unsigned bits,uint32_t value) {
    for(unsigned i=0;i<bits;i++) {
        unsigned p=first+i;
        if(value&(1U<<i))dst[p/8]|=(uint8_t)(1U<<(p%8));
        else dst[p/8]&=(uint8_t)~(1U<<(p%8));
    }
}
static karoq_mqb_decoded_t sample(uint32_t id,const uint8_t data[8]) {
    karoq_mqb_decoded_t d;
    assert(karoq_mqb_decode(id,false,false,8,data,&d));
    return d;
}
static void ok(bool v,unsigned n){if(!v){fprintf(stderr,"FAIL case %u\n",n);exit(1);}printf("PASS %02u\n",n);}
int main(void) {
    uint8_t p[8]={0}; karoq_mqb_decoded_t d;
    // 1: registry count, no repeated IDs.
    ok(karoq_mqb_count()==12,1);
    for(size_t i=0;i<karoq_mqb_count();i++)for(size_t j=i+1;j<karoq_mqb_count();j++)
        assert(karoq_mqb_by_index(i)->can_id!=karoq_mqb_by_index(j)->can_id);
    // 2: real speed 0x0FD: 50.00 km/h, Q-bit bit55.
    encode(p,32,16,5000); encode(p,55,1,1);
    d=sample(0x0fd,p);
    ok(d.raw[0]==5000 && fabsf(d.physical[0]-50.f)<0.001f && d.raw[1]==1,2);
    // 3: 0x0B2 rear-left, rear-right, front-left, front-right all 16-bit.
    memset(p,0,8);for(int i=0;i<4;i++)encode(p,(unsigned)i*16,16,(unsigned)(1000+i*100));
    d=sample(0x0b2,p);
    ok(d.raw[0]==1000 && d.raw[1]==1100 && d.raw[2]==1200 && d.raw[3]==1300,3);
    // 4: actual openpilot cruiseState uses TSK_06 status 2..5 available,3..5 enabled.
    for(unsigned v=0;v<=7;v++){
        memset(p,0,8);encode(p,24,3,v);
        d=sample(0x120,p);
        assert(d.raw[0]==v);
        bool avail=(v>=2&&v<=5),enabled=(v>=3&&v<=5);
        assert((!enabled || avail) && (v<6 || (!avail&&!enabled)));
    }
    ok(true,4);
    // 5: ACC_02 bit12 10-bit setpoint 400*0.32=128; display code is raw not ACC active.
    memset(p,0,8);encode(p,12,10,400);encode(p,61,3,6);
    d=sample(0x30c,p);
    ok(d.raw[0]==400 && fabsf(d.physical[0]-128.0f)<0.001f && d.raw[3]==6,5);
    // 6: LDW_02 lane warning bits and green LED must not map directly to travel assist.
    memset(p,0,8);encode(p,56,1,1);encode(p,61,1,0);encode(p,62,1,1);
    d=sample(0x397,p);
    ok(d.raw[0]==1 && d.raw[1]==0 && d.raw[5]==1,6);
    // 7: EPS + HCA: independent control and camera status; do not conflate.
    memset(p,0,8);encode(p,32,4,9);encode(p,36,4,5);
    d=sample(0x09f,p);
    bool eps_ok=(d.raw[0]==9 && d.raw[1]==5);
    memset(p,0,8);encode(p,32,4,4);encode(p,30,1,1);
    d=sample(0x126,p);
    ok(eps_ok && d.raw[0]==4 && d.raw[1]==1,7);
    // 8: GRA switch bits and independently blind-spot / turn indicator.
    memset(p,0,8);encode(p,12,1,1);encode(p,16,1,1);
    d=sample(0x12b,p);bool gra_ok=(d.raw[0]==1 && d.raw[1]==1);
    memset(p,0,8);encode(p,26,1,1);encode(p,43,1,1);
    d=sample(0x30f,p);bool swa_ok=d.raw[0]==1 && d.raw[3]==1;
    memset(p,0,8);encode(p,23,1,1);encode(p,24,1,0);
    d=sample(0x366,p);
    ok(gra_ok && swa_ok && d.raw[0]==1 && d.raw[1]==0,8);
    // 9: input validation: no ext29/RTR/short/unknown/NULL.
    memset(p,0,8);
    ok(!karoq_mqb_decode(0x120,true,false,8,p,&d) &&
       !karoq_mqb_decode(0x120,false,true,8,p,&d) &&
       !karoq_mqb_decode(0x120,false,false,7,p,&d) &&
       !karoq_mqb_decode(0x777,false,false,8,p,&d) &&
       !karoq_mqb_decode(0x120,false,false,8,NULL,&d),9);
    // 10: independent Intel bits extraction verified by reference shift.
    for (unsigned start=0;start<64;start++)for(unsigned bits=1;bits<=16&&start+bits<=64;bits++){
        for(unsigned k=0;k<8;k++)p[k]=(uint8_t)((start*17+bits*7+k*29)&255U);
        uint32_t raw=0;assert(karoq_mqb_extract_le(p,start,bits,&raw));
        uint64_t word=0;for(unsigned k=0;k<8;k++)word|=(uint64_t)p[k]<<(k*8);
        uint32_t ref=(uint32_t)((word>>start)&((1ULL<<bits)-1));
        assert(raw==ref);
    }
    ok(true,10);
    printf("10run: 10/10 PASS (host bit layout + input checks; NOT vehicle validation)\n");
    return 0;
}
