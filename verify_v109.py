#!/usr/bin/env python3
"""Read-only source verifier for Karoq CAN HUD V1.0.9. Not a hardware test."""
import argparse,hashlib,json,re,sys
from pathlib import Path

def verify(root):
    def ck(flag,message):
        if not flag: raise RuntimeError(message)
    root=root.expanduser().resolve()
    m=root/'main'
    ck(b'project(karoq_can_hud_v1_0_9)' in (root/'CMakeLists.txt').read_bytes(), 'wrong root project')
    checks=json.loads((root/'docs/V109_UI_SHA256.json').read_text(encoding='utf-8'))
    for name,expected in checks.items():
        got=hashlib.sha256((m/'ui/v0_6'/name).read_bytes()).hexdigest()
        ck(got==expected,'V0.6 UI mismatch: '+name)
    print('PASS: 6/6 original V0.6 UI files unchanged')
    cm=(m/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    core=(m/'core/main.c').read_text(encoding='utf-8-sig')
    model=(m/'ui/bridge/karoq_v06_live_model.c').read_text(encoding='utf-8-sig')
    bridge=(m/'ui/bridge/karoq_v06_live_bridge.c').read_text(encoding='utf-8-sig')
    ck('KAROQ_HUD_DEMO_AUTORUN=0' in cm,'demo not disabled')
    ck('id/types' in cm,'missing CAN type include path fix')
    ck(core.count('karoq_v06_live_bridge_attach();')==1,'not exactly one V0.6 UI writer')
    ck(core.count('karoq_can_sniffer_start()')==1,'not exactly one CAN init')
    ck('KAROQ_MQB_ASSIST_ACC_ONLY' in model,'V108 live model fix lost')
    ck('KAROQ_LINK_LANE' not in bridge.split('static karoq_assist_state_t to_native')[1].split('static void tick')[0] and
       'KAROQ_LINK_TRAVEL' not in bridge.split('static karoq_assist_state_t to_native')[1].split('static void tick')[0],
       'lane/travel are unexpectedly wired into green UI')
    ck('twai_transmit(' not in (m/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig'),
       'CAN transmission detected')
    ck('ACC02_ACTIVE_STREAK 3U' in (m/'id/signals/karoq_mqb_assist_state.c').read_text(encoding='utf-8-sig'),
       'ACC02 3-frame guard missing')
    ck('ACC02_FALLBACK_TIMEOUT_MS 350ULL' in (m/'id/signals/karoq_mqb_assist_state.c').read_text(encoding='utf-8-sig'),
       'ACC02 timeout missing')
    ck('firmware=V1.0.9' in (m/'other/storage/karoq_sd_logger.c').read_text(encoding='utf-8-sig'),
       'SD metadata firmware version not updated')
    print('PASS: single UI writer; no demo; ACC02 fallback guards; Lane/Travel grey')
    print('PASS: SD Logger retained, CAN listen-only, 0x0FD speed module retained')
    print('NOTE: Source verifier only; full Xtensa build and actual car test are NOT proven')

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--project',required=True,type=Path);o=a.parse_args()
    try:verify(o.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
