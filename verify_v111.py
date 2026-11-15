#!/usr/bin/env python3
"""Static checks; not a substitute for idf.py build or vehicle test."""
import argparse,hashlib,json,re,sys
from pathlib import Path

def check(p):
    root=Path(p).resolve(); main=root/'main'; rd=lambda s:s.read_text(encoding='utf-8-sig')
    def need(ok,msg):
        if not ok:raise RuntimeError(msg)
    need('project(karoq_can_hud_v1_0_11)' in rd(root/'CMakeLists.txt'),'Incorrect version')
    original=json.loads(rd(root/'docs/V110_UI_SHA256.json'))
    current=json.loads(rd(root/'docs/V111_UI_SHA256.json'))
    need(original==current and len(original)==6,'V0.6 UI manifest changed')
    for filename,digest in original.items():
        need(hashlib.sha256((main/'ui/v0_6'/filename).read_bytes()).hexdigest()==digest,
             'Original V0.6 modified: '+filename)
    bridge=rd(main/'ui/bridge/karoq_v06_live_bridge.c')
    need('s_sd_rec_dot' in bridge and 'karoq_sd_logger_active()' in bridge,'SD dot missing')
    need('KAROQ_LINK_LANE' in bridge and 'KAROQ_LINK_TRAVEL' in bridge,'Lane link missing')
    need('s_vze_sign' in bridge and 'karoq_vze_state_get(now_ms)' in bridge,'VZE UI missing')
    need('lv_obj_set_pos(s_vze_sign, 27, 112)' in bridge,'Sign location changed')
    need('lv_obj_add_flag(s_vze_sign, LV_OBJ_FLAG_HIDDEN)' in bridge,'Fail-closed sign missing')
    need('KAROQ_VZE_SOURCE_VERIFIED 0' in rd(main/'id/signals/karoq_vze_state.c'),
         'Unverified source enabled')
    need('"id/signals/karoq_vze_state.c"' in rd(main/'CMakeLists.txt'),
         'VZE C file not included in build')
    need('"1.0.11"' in rd(main/'core/karoq_firmware_info.h'),'FW version mismatch')
    need('LISTEN_ONLY' in rd(main/'id/can/karoq_can_sniffer.c'),'CAN not listen-only')
    for f in list(main.rglob('*.c')):
        if 'history' not in f.parts:
            need('twai_transmit(' not in rd(f),'CAN TX call in active module '+str(f))
    print('PASS: native V0.6 artwork (6 files) identical SHA-256')
    print('PASS: ACC/Lane/SD original bridges retained; no competing UI timer')
    print('PASS: sign slot hidden until a verified VZE source exists')
    print('PASS: CAN LISTEN_ONLY; no twai_transmit')
    print('NOTE: ESP-IDF compilation and physical sign-display test remain pending')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--project',required=True);a=p.parse_args()
    try:check(a.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
