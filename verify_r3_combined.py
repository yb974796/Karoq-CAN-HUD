#!/usr/bin/env python3
"""Verify final V1.0.14.5 R3 Combined manifests, sources and CAN RX-only."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p): return p.read_text(encoding='utf-8-sig')
def assert_ok(v,m):
    if not v: raise RuntimeError(m)

def verify(p):
    assert_ok('project(karoq_can_hud_v1_0_14_5_r3)' in read(p/'CMakeLists.txt'), 'Not R3 CMake project')
    assert_ok('"1.0.14.5-R3"' in read(p/'main/core/karoq_firmware_info.h'), 'Not R3 version header')
    for mf in ('docs/V1145_PATCH_SHA256.json', 'docs/V1145_R3_ACC_PATCH_SHA256.json'):
        for rel,digest in json.loads(read(p/mf)).items():
            assert_ok(sha(p/rel)==digest,'Checksum mismatch: '+rel)
    assert_ok(len(json.loads(read(p/'docs/V1145_R3_ACC_PATCH_SHA256.json')))==2,'R3 changed file count wrong')
    acc_rel=read(p/'docs/V1145_R3_ACC_SOURCE_PATH.txt').strip()
    acc=read(p/acc_rel)
    ui=read(p/'main/ui/layout/karoq_reference_layout.c')
    native=read(p/'main/ui/v0_6/karoq_hud.c')
    for tok in ('ahead<=150U','karoq_ui_acc_set_snapshot'):
        assert_ok(tok in acc,'ACC skew code missing: '+tok)
    for tok in ('ACC_STALE_HOLD_MS','ACC_FRESH_OFF_CONFIRM_MS','KAROQ_ACC_UI_DIAG','s_acc_icon_src_writes','KAROQ_NAV_UTURN_LEFT'):
        assert_ok(tok in ui,'UI ACC/NAV fix missing: '+tok)
    assert_ok('lv_obj_set_pos(s_original_speed' not in ui,'Dual owner of speed layout')
    assert_ok('karoq_speed_geometry(' in native,'R1 native speed geometry missing')
    assert_ok((p/'main/ui/layout/karoq_speed_geometry.h').is_file(),'R1 speed geometry missing')
    assert_ok(sha(p/'main/ui/layout/karoq_acc_dual_palette.c')==
              read(p/'docs/V1145_R2_ACC_PALETTE_SHA256.txt').strip(),'R2 ACC gauge changed')
    originals=json.loads(read(p/'docs/V1144_UI_SHA256.json'))
    cur=json.loads(read(p/'docs/V1145_NATIVE_SHA256.json'))
    assert_ok(set(originals)==set(cur),'V0.6 native inventory changed')
    for name,digest in cur.items():
        assert_ok(sha(p/'main/ui/v0_6'/name)==digest, 'V0.6 source differs: '+name)
        if name!='karoq_hud.c': assert_ok(originals[name]==digest,'V0.6 non-speed source changed')
    assert_ok('LISTEN_ONLY' in read(p/'main/id/can/karoq_can_sniffer.c'),'CAN listen-only lost')
    for name in ('karoq_can_sniffer.c','karoq_can_dispatcher.c'):
        assert_ok('twai_transmit(' not in read(p/'main/id/can'/name),'Unexpected CAN TX: '+name)
    assert_ok(read(p/'main/ui/bridge/karoq_v06_live_bridge.c').count('lv_timer_create(tick, 100, NULL)')==1,
              'Expected one LVGL bridge timer')
    print('PASS: R1 stable speed + R2 gauge + R3 flicker fix + NAV + V0.6 + RX only')
    print('NOTE: full ESP-IDF target build and actual LCD/vehicle tests are still required')

if __name__=='__main__':
    try:
        ap=argparse.ArgumentParser();ap.add_argument('--project',type=Path,required=True)
        args=ap.parse_args();verify(args.project.expanduser().resolve())
    except Exception as exc:
        print('FAIL:',exc,file=sys.stderr);sys.exit(1)
