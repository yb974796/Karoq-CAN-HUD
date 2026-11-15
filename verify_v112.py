#!/usr/bin/env python3
"""Static source validation. Run idf.py build on Windows to validate the target."""
import argparse,hashlib,json,sys,re
from pathlib import Path
UI=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h',
    'karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def need(x,m):
    if not x: raise RuntimeError(m)
def rd(p): return p.read_text(encoding='utf-8-sig')
def verify(path):
    p=Path(path).resolve(); main=p/'main'
    need('project(karoq_can_hud_v1_0_12)' in rd(p/'CMakeLists.txt'),'Version error')
    old=json.loads(rd(p/'docs/V111_UI_SHA256.json'))
    new=json.loads(rd(p/'docs/V112_UI_SHA256.json'))
    need(old==new and set(new)==set(UI),'V0.6 SHA256 manifest mismatch')
    for f in UI:
        need(hashlib.sha256((main/'ui/v0_6'/f).read_bytes()).hexdigest()==new[f],
             'Native UI modified: '+f)
    snif=rd(main/'id/can/karoq_can_sniffer.c')
    need('LISTEN_ONLY' in snif and 'karoq_can_dispatcher_receive(' in snif,
         'Original RX path not forwarded to traffic dispatcher')
    need('karoq_sd_logger_receive(' not in snif,
         'Two receive paths or outdated sniffer call')
    disp=rd(main/'id/can/karoq_can_dispatcher.c')
    need('karoq_sd_logger_receive(msg, wait_ticks)' in disp and
         'karoq_ts_vza_receive(' in disp,'Dispatch fanout missing')
    bridge=rd(main/'ui/bridge/karoq_v06_live_bridge.c')
    need('karoq_ts_service_snapshot(now_ms)' in bridge and
         's_sd_rec_dot' in bridge and 'KAROQ_LINK_LANE' in bridge and
         'KAROQ_LINK_TRAVEL' in bridge,'V0.6 UI or traffic binding changed')
    need('lv_obj_set_pos(s_vze_sign, 27, 112)' in bridge,
         'V0.6 traffic sign location changed')
    need('char value[8]' in bridge,'V1.0.11 fixed snprintf buffer lost')
    need('"1.0.12"' in rd(main/'core/karoq_firmware_info.h'),'FW macro not updated')
    need('firmware=V1.0.12' in rd(main/'other/storage/karoq_sd_logger.c'),
         'SD META firmware stale')
    need('KAROQ_TS_VZE_TRIAL_DISPLAY 1' in rd(main/'id/traffic_sign/karoq_ts_vza_decoder.c'),
         'Trial speed sign display unexpectedly disabled')
    cmake=rd(main/'CMakeLists.txt')
    for r in ('id/can/karoq_can_dispatcher.c','id/traffic_sign/karoq_ts_bap.c',
              'id/traffic_sign/karoq_ts_vza_decoder.c',
              'other/traffic_sign/karoq_ts_service.c',
              'other/traffic_sign/karoq_ts_carplay_adapter.c'):
        need('"'+r+'"' in cmake,'CMake missing '+r)
    for folder in ('"id/traffic_sign"','"other/traffic_sign"'):
        need(folder in cmake,'CMake includes missing '+folder)
    for c in main.rglob('*.c'):
        need('twai_transmit(' not in rd(c),'CAN TX found in '+str(c))
    print('PASS: native six V0.6 files byte-identical')
    print('PASS: one CAN RX path, SD + VZA dispatcher, original ACC/Lane/SD retained')
    print('PASS: BAP v1.0.12 sources and CarPlay adapter registered')
    print('PASS: native LVGL speed-limit UI bound to independent Traffic Sign Service')
    print('PASS: no twai_transmit, SD metadata V1.0.12')
    print('NOTE: target ESP-IDF compile & in-car VZA display NOT tested here')
if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--project',required=True);x=a.parse_args()
    try:verify(x.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
