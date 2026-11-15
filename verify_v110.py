#!/usr/bin/env python3
"""Static source verifier only; does NOT replace ESP-IDF build or real vehicle test."""
from pathlib import Path
import argparse, hashlib, json, sys


def verify(root):
    root=Path(root).resolve();m=root/'main'
    def need(x,msg):
        if not x:raise RuntimeError(msg)
    def rd(p):return p.read_text(encoding='utf-8-sig')
    need('project(karoq_can_hud_v1_0_10)' in rd(root/'CMakeLists.txt'),'Wrong project name')
    needed=json.loads(rd(root/'docs/V110_UI_SHA256.json'))
    need(len(needed)==6,'Expected 6 original UI files')
    for name,expected in needed.items():
        actual=hashlib.sha256((m/'ui/v0_6'/name).read_bytes()).hexdigest()
        need(actual==expected,'Original V0.6 file changed: '+name)
    print('PASS: all 6 original 640x172 V0.6 UI source files preserved (SHA256)')
    core=rd(m/'core/main.c');bridge=rd(m/'ui/bridge/karoq_v06_live_bridge.c')
    model=rd(m/'ui/bridge/karoq_v06_live_model.c')
    engine=rd(m/'id/signals/karoq_mqb_assist_state.c')
    header=rd(m/'id/signals/karoq_mqb_assist_state.h')
    sd=rd(m/'other/storage/karoq_sd_logger.c')
    cmake=rd(m/'CMakeLists.txt')
    need(core.count('karoq_v06_live_bridge_attach();')==1,'Multiple native UI writers')
    need('KAROQ_HUD_DEMO_AUTORUN=0' in cmake,'V0.6 demo enabled')
    need('id/types' in cmake,'Missing original type include fix')
    for text in ('KAROQ_LINK_LANE','KAROQ_LINK_TRAVEL','karoq_sd_logger_active()',
                 'lv_obj_create(lv_screen_active())','lv_obj_set_pos(s_sd_rec_dot, 626, 5)'):
        need(text in bridge,text+' not connected in native bridge')
    need('s_sd_rec_dot' in bridge and 'LV_OBJ_FLAG_HIDDEN' in bridge,'SD dot state missing')
    for text in ('assist.lane_active','assist.acc_active && assist.lane_active'):
        need(text in model,'Live model lacks independent lane/ACC: '+text)
    for text in ('EPS_ACTIVE_TIMEOUT_MS 1500ULL','EPS_ACTIVE_CONFIRM_FRAMES 2U',
                 'out.lane_active=out.lane_known','e->eps.eps_active_streak',
                 'ACC02_ACTIVE_STREAK 3U','ACC02_FALLBACK_TIMEOUT_MS 350ULL'):
        need(text in engine,'Engine missing: '+text)
    need('bool lane_active;' in header and 'bool lane_known;' in header,'Engine UI flags missing')
    need('firmware=V1.0.10' in sd,'SD metadata not updated')
    need('"1.0.10"' in rd(m/'core/karoq_firmware_info.h'),'Version macro missing')
    need('LISTEN_ONLY' in rd(m/'id/can/karoq_can_sniffer.c'),'CAN not listen-only')
    for f in m.rglob('*.c'):
        # Cross check in shipped (not history) project sources
        need('twai_transmit(' not in rd(f), 'Unexpected CAN TX call: '+str(f))
    print('PASS: EPS=5 2-frame Lane + ACC02 retained, BOTH display correctly wired')
    print('PASS: SD recorder indicator reads actual logger state in one native LVGL writer')
    print('PASS: CAN listen-only, metadata v1.0.10, no CAN transmit call')
    print('NOTE: Source verification, NOT a full ESP-IDF build or car test')


if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--project',required=True)
    args=a.parse_args()
    try:verify(args.project)
    except Exception as e:
        print('FAIL:',e,file=sys.stderr);sys.exit(1)
