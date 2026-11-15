#!/usr/bin/env python3
"""Static project checks (not ESP-IDF compilation or hardware acceptance)."""
import argparse,hashlib,json,pathlib,sys
UI=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h',
    'karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def main(p):
 def rd(rel):return (p/rel).read_text(encoding='utf-8-sig')
 original=json.loads(rd('docs/V1141_UI_SHA256.json'))
 assert set(original)==set(UI)
 for n in UI:
  assert hashlib.sha256((p/'main/ui/v0_6'/n).read_bytes()).hexdigest()==original[n],n
 assert 'project(karoq_can_hud_v1_0_14_1)' in rd('CMakeLists.txt')
 for rel,key in [('main/id/navigation/karoq_oem_nav_decoder.c','little-endian uint32 distance'),
       ('main/id/signals/karoq_ui_acc_set.c','KAROQ_ACC_UI_ACTIVE'),
       ('main/ui/layout/karoq_reference_layout.c','KAROQ_ACC3_V1141'),
       ('main/ui/layout/karoq_acc_green_palette.c','karoq_acc_green_cruise'),
       ('main/ui/layout/karoq_large_sign_assets.c','karoq_large_sign_50')]:
  assert key in rd(rel),rel
 cmake=rd('main/CMakeLists.txt')
 assert cmake.count('"ui/layout/karoq_acc_green_palette.c"')==1
 assert cmake.count('"ui/layout/karoq_large_sign_assets.c"')==1
 assert cmake.count('"ui/layout/karoq_reference_layout.c"')==1
 assert cmake.count('"id/navigation/karoq_oem_nav_decoder.c"')==1
 assert cmake.count('"id/signals/karoq_ui_acc_set.c"')==1
 bridge=rd('main/ui/bridge/karoq_v06_live_bridge.c')
 assert bridge.count('lv_timer_create(tick, 100, NULL)')==1
 assert rd('main/ui/layout/karoq_reference_layout.c').count('karoq_oem_nav_tick(')==1
 assert rd('main/id/can/karoq_can_dispatcher.c').count('karoq_oem_nav_receive(')==1
 for rel in ('main/id/can/karoq_can_sniffer.c','main/id/can/karoq_can_dispatcher.c'):
  assert 'twai_transmit(' not in rd(rel)
 assert 'LISTEN_ONLY' in rd('main/id/can/karoq_can_sniffer.c')
 assert 'firmware=V1.0.14.1' in rd('main/other/storage/karoq_sd_logger.c')
 print('PASS: UI V0.6 SHA256, NAV decode, ACC 3-state, palette, singleton timer, RX-only')
 print('NOTE: Windows ESP-IDF build and on-car LCD test still required')
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--project',required=True,type=pathlib.Path)
 a=ap.parse_args()
 try:main(a.project.resolve())
 except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
