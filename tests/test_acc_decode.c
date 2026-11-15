#include "karoq_can_acc_probe.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    karoq_can_frame_t f = {0}; karoq_acc_snapshot_t acc = {0};
    f.id = 0x30C; f.dlc = 8;
    const uint8_t parked[8] = {0xA5,0xF1,0x3F,0x00,0x60,0x30,0x00,0x20};
    memcpy(f.data, parked, 8);
    assert(karoq_acc_decode(&f,&acc));
    assert(acc.target_raw == 1023 && !acc.target_valid);
    assert(acc.status_display == 1 && acc.status_primary == 0);
    f.data[1] = 0x00; f.data[2] = 0x19; // 0x190 = 400, target 128 km/h
    assert(karoq_acc_decode(&f,&acc));
    assert(acc.target_valid && fabsf(acc.target_kmh - 128.0f) < 0.001f);
    f.data[2] = 0x59; // primary raw = 1
    assert(karoq_acc_decode(&f,&acc) && acc.status_primary == 1);
    f.dlc = 7; assert(!karoq_acc_decode(&f,&acc));
    f.dlc = 8; f.extended = true; assert(!karoq_acc_decode(&f,&acc));
    f.extended = false; f.rtr = true; assert(!karoq_acc_decode(&f,&acc));
    f.rtr = false; f.id = 0x30B; assert(!karoq_acc_decode(&f,&acc));
    puts("PASS ACC_02 raw parser 0x30C");
    return 0;
}
