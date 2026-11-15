#!/usr/bin/env python3
"""Integrity check of installed files, including V0.6 and CAN RX-only invariants."""
import argparse,hashlib,json,pathlib,sys
ORIGINAL=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h',
          'karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return p.read_text(encoding='utf-8-sig')
def verify(r):
 assert 'project(karoq_can_hud_v1_0_14_4)' in read(r/'CMakeLists.txt')
 recorded=json.loads(read(r/'docs/V1144_PATCH_SHA256.json'))
 for rel,digest in recorded.items():assert sha(r/rel)==digest,(rel,'patch hash changed')
 originals=json.loads(read(r/'docs/V1144_UI_SHA256.json'))
 assert set(originals)==set(ORIGINAL)
 for name in ORIGINAL:assert sha(r/'main/ui/v0_6'/name)==originals[name],name
 cm=read(r/'main/CMakeLists.txt')
 for name in ('karoq_reference_layout','karoq_nav_clean_assets','karoq_acc_dual_palette'):
  assert cm.count(f'"ui/layout/{name}.c"')==1,name
 header=read(r/'main/other/navigation/karoq_nav_service.h')
 assert 'KAROQ_NAV_UTURN_LEFT = 12' in header and 'KAROQ_NAV_UTURN_RIGHT = 13' in header
 for rel in ('main/id/can/karoq_can_sniffer.c','main/id/can/karoq_can_dispatcher.c'):
  assert 'twai_transmit(' not in read(r/rel)
 assert 'LISTEN_ONLY' in read(r/'main/id/can/karoq_can_sniffer.c')
 assert read(r/'main/ui/bridge/karoq_v06_live_bridge.c').count('lv_timer_create(tick, 100, NULL)')==1
 assert '"1.0.14.4"' in read(r/'main/core/karoq_firmware_info.h')
 print('PASS: installed source SHA256, UI V0.6 unchanged, NAV enum, CMake, CAN RX-only, one LVGL timer')
 print('NOTE: Host C11 tests pass separately; real ESP-IDF build/LCD still required')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--project',type=pathlib.Path,required=True)
 args=p.parse_args()
 try:verify(args.project.resolve())
 except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
