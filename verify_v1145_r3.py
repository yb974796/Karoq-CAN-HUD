#!/usr/bin/env python3
"""Verify the R3 patch and preservation of R1 speed/R2 gauge after upgrade."""
import argparse,hashlib,json,pathlib,sys
V06=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h','karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def read(p):return p.read_text(encoding='utf-8-sig')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def need(ok,msg):
 if not ok:raise RuntimeError(msg)
def verify(p):
 need('project(karoq_can_hud_v1_0_14_5_r3)' in read(p/'CMakeLists.txt'),'Project version is not R3')
 need('"1.0.14.5-R3"' in read(p/'main/core/karoq_firmware_info.h'),'Firmware version is not R3')
 for manifest in ['docs/V1145_PATCH_SHA256.json','docs/V1145_R3_ACC_PATCH_SHA256.json']:
  for rel,digest in json.loads(read(p/manifest)).items():
   need(sha(p/rel)==digest,'Checksum mismatch: '+rel)
 r3=json.loads(read(p/'docs/V1145_R3_ACC_PATCH_SHA256.json'))
 need(len(r3)==2,'R3 patch should contain exactly two source changes')
 acc_path=read(p/'docs/V1145_R3_ACC_SOURCE_PATH.txt').strip()
 need(acc_path in r3,'ACC timestamp source missing from R3 manifest')
 need('ahead<=150U' in read(p/acc_path),'ACC skew protection absent')
 ui=read(p/'main/ui/layout/karoq_reference_layout.c')
 for token in ('ACC_FRESH_OFF_CONFIRM_MS','ACC_STALE_HOLD_MS','KAROQ_ACC_UI_DIAG','s_acc_icon_src_writes'):
  need(token in ui,'UI diagnostic/debounce missing: '+token)
 need('lv_obj_set_pos(s_original_speed' not in ui,'Old speed-position second writer returned')
 need('karoq_speed_geometry(' in read(p/'main/ui/v0_6/karoq_hud.c'),'Stable native speed fix missing')
 for n,checksum in json.loads(read(p/'docs/V1145_NATIVE_SHA256.json')).items():
  need(sha(p/'main/ui/v0_6'/n)==checksum,'Native V0.6 modified: '+n)
 pal=p/'main/ui/layout/karoq_acc_dual_palette.c'
 need(sha(pal)==read(p/'docs/V1145_R2_ACC_PALETTE_SHA256.txt').strip(),
      'R2 original speedometer gauge replaced')
 need('KAROQ_NAV_UTURN_LEFT' in ui,'NAV left U-turn missing')
 need('LISTEN_ONLY' in read(p/'main/id/can/karoq_can_sniffer.c'),'CAN Listen-only lost')
 for rel in ('main/id/can/karoq_can_sniffer.c','main/id/can/karoq_can_dispatcher.c'):
  need('twai_transmit(' not in read(p/rel),'Forbidden CAN TX: '+rel)
 need(read(p/'main/ui/bridge/karoq_v06_live_bridge.c').count('lv_timer_create(tick, 100, NULL)')==1,
      'Original LVGL timer count changed')
 print('PASS: R3 ACC skew/debounce, original R2 gauge, R1 speed, NAV, V0.6 and CAN RX-only intact')
 print('NOTE: target ESP-IDF build and real LCD tests have not been run')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--project',required=True,type=pathlib.Path)
 a=ap.parse_args();verify(a.project.resolve())
if __name__=='__main__':
 try:main()
 except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
