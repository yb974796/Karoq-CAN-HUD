#!/usr/bin/env python3
"""Non-destructive integrity validation for V1.0.14.2."""
import argparse,hashlib,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
REL=(
 'main/ui/layout/karoq_reference_layout.c',
 'main/id/navigation/karoq_oem_nav_decoder.c'
)
ORIGINAL=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h',
          'karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def validate(src):
 assert 'project(karoq_can_hud_v1_0_14_2)' in (src/'CMakeLists.txt').read_text(encoding='utf-8-sig')
 assert (src/'main/core/karoq_firmware_info.h').is_file()
 patch_hashes=json.loads((src/'docs/V1142_PATCH_SHA256.json').read_text())
 assert set(patch_hashes)==set(REL)
 for rel in REL:
  assert sha(src/rel)==patch_hashes[rel],(rel,'modified vs patch')
 orig=json.loads((src/'docs/V1142_UI_SHA256.json').read_text())
 assert set(orig)==set(ORIGINAL)
 for name in ORIGINAL:
  assert sha(src/'main/ui/v0_6'/name)==orig[name],name
 for key in ('main/id/can/karoq_can_sniffer.c','main/id/can/karoq_can_dispatcher.c'):
  assert 'twai_transmit(' not in (src/key).read_text(encoding='utf-8-sig'),key
 assert 'LISTEN_ONLY' in (src/'main/id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
 print('PASS: patch files identical; original V0.6 files unchanged; CAN RX only')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--project',required=True,type=pathlib.Path)
 args=p.parse_args()
 try:validate(args.project.resolve())
 except Exception as e: print('FAIL:',repr(e),file=sys.stderr);sys.exit(1)
