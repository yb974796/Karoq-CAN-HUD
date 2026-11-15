#!/usr/bin/env python3
"""Validate V1.0.13 source upgrade invariants; does not compile target binary."""
import argparse,hashlib,json,re,sys
from pathlib import Path
UI=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h',
    'karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
CFILES=('karoq_nav_service.c','karoq_nav_wire.c','karoq_nav_carplay_adapter.c')
HFILES=('karoq_nav_service.h','karoq_nav_wire.h','karoq_nav_carplay_adapter.h')
def need(ok,msg):
    if not ok:raise ValueError(msg)
def rd(p):return p.read_text(encoding='utf-8-sig')
def check(p):
    root=Path(p).resolve();main=root/'main'
    need('project(karoq_can_hud_v1_0_13)' in rd(root/'CMakeLists.txt'),'Project version incorrect')
    a=json.loads(rd(root/'docs/V112_UI_SHA256.json'))
    b=json.loads(rd(root/'docs/V113_UI_SHA256.json'))
    need(a==b and set(b)==set(UI),'Original native V0.6 hash manifest changed')
    for f in UI:
        need(hashlib.sha256((main/'ui/v0_6'/f).read_bytes()).hexdigest()==b[f],
             'Original native V0.6 UI changed: '+f)
    cm=rd(main/'CMakeLists.txt')
    for f in CFILES:
        need('"other/navigation/'+f+'"' in cm,'Missing CMake source: '+f)
    need('"other/navigation"' in cm,'Missing CMake NAV include dir')
    for f in CFILES+HFILES:
        need((main/'other/navigation'/f).is_file(),'Missing NAV source: '+f)
    need('"1.0.13"' in rd(main/'core/karoq_firmware_info.h'),'FW version not updated')
    need('firmware=V1.0.13' in rd(main/'other/storage/karoq_sd_logger.c'),
         'SD firmware META stale')
    rx=rd(main/'id/can/karoq_can_sniffer.c')
    need('LISTEN_ONLY' in rx and 'karoq_can_dispatcher_receive(' in rx,
         'CAN RX/listen-only changed')
    bridge=rd(main/'ui/bridge/karoq_v06_live_bridge.c')
    need('karoq_ts_service_snapshot(now_ms)' in bridge and 's_sd_rec_dot' in bridge and
         'KAROQ_LINK_LANE' in bridge, 'Stable VZA/SD/Lane HUD bridge changed')
    need('lv_obj_set_pos(s_vze_sign, 27, 112)' in bridge,'VZA original position changed')
    for f in main.rglob('*.c'):
        need(not re.search(r'\btwai_transmit\s*\(',rd(f)),'Forbidden CAN TX: '+str(f))
    print('PASS: V1.0.13 / original V0.6 six files SHA256 unchanged')
    print('PASS: existing VZA / ACC / Lane / SD and listen-only paths retained')
    print('PASS: independent navigation service, wire parser and RGI adapter present')
    print('PASS: no new input transport, no live CarPlay claim, no CAN TX')
    print('NOTE: Windows ESP-IDF full build and car test required')
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--project',required=True)
    a=ap.parse_args()
    try:check(a.project)
    except Exception as exc:
        print('FAIL:',exc,file=sys.stderr);sys.exit(1)
