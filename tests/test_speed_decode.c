#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "karoq_can_speed.h"

int main(void)
{
    karoq_can_frame_t f={0}; karoq_speed_snapshot_t s={0};
    f.id=0x0FD; f.dlc=8;
    const uint8_t stationary[8]={0x0B,0xD0,0x1F,0x80,0,0,0,0};
    memcpy(f.data, stationary, 8);
    assert(karoq_speed_decode(&f,&s) && fabsf(s.kmh)<0.0001f);
    // Test 50.00 km/h: raw 5000 = 0x1388 little endian in bytes 4/5.
    f.data[4]=0x88; f.data[5]=0x13;
    assert(karoq_speed_decode(&f,&s) && fabsf(s.kmh-50.0f)<0.001f);
    f.data[6]=0x80;
    assert(karoq_speed_decode(&f,&s) && s.esp_qbit);
    // Cluster DBC uses 10-bit little-endian field at bit48, factor 0.32.
    f.id=0x30B; memset(f.data,0,8); f.data[6]=0xFA; f.data[7]=0x00;
    assert(karoq_speed_decode(&f,&s) && fabsf(s.kmh-80.0f)<0.001f);
    f.data[6]=0x00; f.data[7]=0x01;
    assert(karoq_speed_decode(&f,&s) && fabsf(s.kmh-81.92f)<0.001f);
    f.dlc=7; assert(!karoq_speed_decode(&f,&s));
    f.dlc=8; f.extended=true; assert(!karoq_speed_decode(&f,&s));
    f.extended=false; f.rtr=true; assert(!karoq_speed_decode(&f,&s));
    f.rtr=false; f.id=0x0FD; f.data[4]=0xff; f.data[5]=0xff;
    assert(!karoq_speed_decode(&f,&s));
    puts("PASS: 0x0FD and 0x30B signal decode tests");
    return 0;
}
