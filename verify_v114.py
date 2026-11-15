#!/usr/bin/env python3
"""Verify V1.0.14 static code-integrity; NOT ESP-IDF hardware acceptance."""
import argparse,hashlib,json,pathlib,sys
UI=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h',
    'karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def main(root):
    def read(p):return (root/p).read_text(encoding='utf-8-sig')
    manifest=json.loads(read('docs/V114_UI_SHA256.json'))
    need(set(manifest)==set(UI),'UI hashes incomplete')
    for name in UI:
        need(hashlib.sha256((root/'main/ui/v0_6'/name).read_bytes()).hexdigest()==manifest[name],
             'Original V0.6 changed: '+name)
    need('project(karoq_can_hud_v1_0_14)' in read('CMakeLists.txt'),'Version mismatch')
    dispatcher=read('main/id/can/karoq_can_dispatcher.c')
    layout=read('main/ui/layout/karoq_reference_layout.c')
    bridge=read('main/ui/bridge/karoq_v06_live_bridge.c')
    sources=read('main/CMakeLists.txt')
    sniffer=read('main/id/can/karoq_can_sniffer.c')
    need(dispatcher.count('KAROQ_OEM_NAV_V114')==1,'Missing dispatcher hook')
    need(layout.count('KAROQ_OEM_NAV_V114')==1,'Missing UI hook')
    need(dispatcher.count('karoq_oem_nav_receive(')==1,'Extra/missing OEM receive')
    need(layout.count('karoq_oem_nav_tick(')==1,'Extra/missing OEM UI tick')
    need(bridge.count('lv_timer_create(tick, 100, NULL)')==1,
         'Additional LVGL timers detected')
    need('karoq_ts_vza_receive(' in dispatcher and
         'karoq_ui_acc_set_receive(' in dispatcher,'Known CAN consumers missing')
    need('karoq_nav_service_snapshot(now_ms)' in layout,'NAV renderer broken')
    need('LISTEN_ONLY' in sniffer and 'karoq_can_dispatcher_receive(' in sniffer,
         'Original listen-only CAN receive task missing')
    for file in ('karoq_oem_nav_bap.c','karoq_oem_nav_decoder.c'):
        need('"id/navigation/'+file+'"' in sources,'CMake missing '+file)
        need((root/'main/id/navigation'/file).is_file(),'Missing '+file)
    for t in (dispatcher,layout,sniffer):
        need('twai_transmit(' not in t,'CAN TX function detected')
    need('firmware=V1.0.14' in read('main/other/storage/karoq_sd_logger.c'),
         'SD META not updated')
    print('PASS: V1.0.14 original V0.6 UI SHA256 unchanged')
    print('PASS: Navigation BAP hooks, existing GUI timer, ACC/VZA and CAN RX-only')
    print('NOTE: ESP-IDF compile and real OEM navigation route not validated here')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--project',required=True,type=pathlib.Path)
    a=p.parse_args()
    try:main(a.project.resolve())
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
