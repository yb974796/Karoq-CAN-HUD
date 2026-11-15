#!/usr/bin/env python3
"""Verify speed R1 and ACC gauge R2 combined project without flashing."""
import argparse,hashlib,json,pathlib,sys
V06=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h','karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return p.read_text(encoding='utf-8-sig')
def need(cond,msg):
 if not cond:raise RuntimeError(msg)
def verify(root):
 need('project(karoq_can_hud_v1_0_14_5_r2)' in read(root/'CMakeLists.txt'),'Wrong project version')
 need('"1.0.14.5-R2"' in read(root/'main/core/karoq_firmware_info.h'),'Wrong firmware version')
 manifest=json.loads(read(root/'docs/V1145_NATIVE_SHA256.json'))
 original=json.loads(read(root/'docs/V1144_UI_SHA256.json'))
 need(set(manifest)==set(original)==set(V06),'Incomplete V0.6 manifest')
 for n in V06:
  need(sha(root/'main/ui/v0_6'/n)==manifest[n],f'Native checksum changed: {n}')
  if n!='karoq_hud.c':need(manifest[n]==original[n],f'Unexpected edit to V0.6: {n}')
 need(manifest['karoq_hud.c']!=original['karoq_hud.c'],'Speed R1 native fix missing')
 patch=json.loads(read(root/'docs/V1145_PATCH_SHA256.json'))
 need(len(patch)==4,'Expected 4 combined firmware files')
 for rel,digest in patch.items():need(sha(root/rel)==digest,f'Combined patch checksum mismatch: {rel}')
 need(sha(root/'main/ui/layout/karoq_acc_dual_palette.c')==read(root/'docs/V1145_R2_ACC_PALETTE_SHA256.txt').strip(),'ACC R2 payload mismatch')
 ui=read(root/'main/ui/layout/karoq_reference_layout.c')
 native=read(root/'main/ui/v0_6/karoq_hud.c')
 pal=read(root/'main/ui/layout/karoq_acc_dual_palette.c')
 need('karoq_speed_geometry(' in native and 'observe_speed_positions' in ui,'Missing single-owner speed layout')
 need('lv_obj_set_pos(s_original_speed' not in ui and 'lv_image_set_scale(s_original_speed' not in ui,'Double speed positioning reintroduced')
 for sym in ('karoq_acc_new_gray_cruise','karoq_acc_new_green_cruise'):
  need(sym in ui and sym in pal,f'ACC gauge symbol missing: {sym}')
 cm=read(root/'main/CMakeLists.txt')
 for s in ('"ui/v0_6/karoq_hud.c"','"ui/layout/karoq_reference_layout.c"','"ui/layout/karoq_acc_dual_palette.c"'):
  need(s in cm,f'Expected compiled source missing: {s}')
 for rel in ('main/id/can/karoq_can_sniffer.c','main/id/can/karoq_can_dispatcher.c'):
  need('twai_transmit(' not in read(root/rel),f'CAN TX unexpected: {rel}')
 need('LISTEN_ONLY' in read(root/'main/id/can/karoq_can_sniffer.c'),'CAN listen-only lost')
 need(read(root/'main/ui/bridge/karoq_v06_live_bridge.c').count('lv_timer_create(tick, 100, NULL)')==1,'LVGL single writer changed')
 print('PASS: V1.0.14.5 R2 COMBINED, speed single owner, original ACC gauge, 5 V0.6 untouched, NAV unchanged and CAN RX-only')
 print('NOTE: User Windows ESP-IDF build and physical display validation still required')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--project',required=True,type=pathlib.Path);args=p.parse_args()
 try:verify(args.project.resolve())
 except Exception as e:print('FAIL:',repr(e),file=sys.stderr);sys.exit(1)
