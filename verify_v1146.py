#!/usr/bin/env python3
"""V1.0.14.6 R1 installed-project integrity and receive-only guard."""
from pathlib import Path
import argparse,hashlib,json,sys

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return p.read_text(encoding='utf-8-sig')
def need(v,msg):
    if not v:raise RuntimeError(msg)
def verify(p):
    need(p.is_dir(),f'No project: {p}')
    need('project(karoq_can_hud_v1_0_14_6_r1)' in read(p/'CMakeLists.txt'),'Unexpected CMake version')
    need('"1.0.14.6-R1"' in read(p/'main/core/karoq_firmware_info.h'),'Unexpected FW version')
    need((p/'sdkconfig').is_file(),'Configured project required')
    patched=json.loads(read(p/'docs/V1146_PATCH_SHA256.json'))
    need(len(patched)==2, 'Expected exactly two UI source patches')
    for rel,digest in patched.items(): need(sha(p/rel)==digest,'Optimized source modified: '+rel)
    native=json.loads(read(p/'docs/V1146_NATIVE_SHA256.json'))
    need(len(native)==6,'V0.6 inventory incomplete')
    old=json.loads(read(p/'docs/V1144_UI_SHA256.json'))
    for name,digest in native.items():
        need(sha(p/'main/ui/v0_6'/name)==digest,'Native source modified: '+name)
        if name!='karoq_hud.c':need(digest==old[name],'Unapproved V0.6 file change: '+name)
    ui=read(p/'main/ui/layout/karoq_reference_layout.c')
    speed=read(p/'main/ui/v0_6/karoq_hud.c')
    acc_rel=read(p/'docs/V1145_R3_ACC_SOURCE_PATH.txt').strip()
    acc=read(p/acc_rel)
    pal=p/'main/ui/layout/karoq_acc_dual_palette.c'
    need(sha(pal)==read(p/'docs/V1145_R2_ACC_PALETTE_SHA256.txt').strip(),'ACC gauge not preserved')
    need('ahead<=150U' in acc and 'ACC_STALE_HOLD_MS' in ui,'R3 ACC skew/hold fix missing')
    need('s_nav_last_glyph' in ui and 's_speed_mismatch_active' in ui,'Selective updates absent')
    need('s_last_speed[i]!=text[i]' in speed and 'char speed[6];' in speed,'Vehicle speed optimization missing')
    need('lv_obj_set_pos(s_original_speed' not in ui,'Speed position written by second owner')
    can=p/'main/id/can/karoq_can_sniffer.c'
    need('LISTEN_ONLY' in read(can),'CAN listen-only missing')
    for f in ('karoq_can_sniffer.c','karoq_can_dispatcher.c'):
        need('twai_transmit(' not in read(p/'main/id/can'/f),'TX attempt found: '+f)
    need(read(p/'main/ui/bridge/karoq_v06_live_bridge.c').count('lv_timer_create(tick, 100, NULL)')==1,'Unexpected LVGL timer count')
    print('PASS: V1.0.14.6 R1 20RUN stability; native speed single-owner; NAV/VZA/ACC; CAN LISTEN_ONLY; project intact')
    print('NOTE: target ESP-IDF build and vehicle/LCD validation must still be done on hardware.')
if __name__=='__main__':
    try:
        ap=argparse.ArgumentParser();ap.add_argument('--project',required=True,type=Path)
        args=ap.parse_args();verify(args.project.expanduser().resolve())
    except Exception as exc: print('FAIL:',exc,file=sys.stderr);sys.exit(1)
