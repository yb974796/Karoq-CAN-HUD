#!/usr/bin/env python3
"""Portable self-check for installed V1.0.14.3 R1 source project."""
import argparse,hashlib,json,pathlib,sys
UI='main/ui/layout/karoq_reference_layout.c'
NAV='main/id/navigation/karoq_oem_nav_decoder.c'
NATIVE=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c',
        'karoq_hud_assets.h','karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path):return path.read_text(encoding='utf-8-sig')
def main(p):
    assert 'project(karoq_can_hud_v1_0_14_3)' in read(p/'CMakeLists.txt')
    assert '"1.0.14.3"' in read(p/'main/core/karoq_firmware_info.h')
    expected=json.loads(read(p/'docs/V1143_EXPECTED_SHA256.json'))
    for rel in (UI,NAV):assert sha(p/rel)==expected[rel],rel
    orig=json.loads(read(p/'docs/V1143_UI_SHA256.json'))
    assert set(orig)==set(NATIVE)
    for n in NATIVE:assert sha(p/'main/ui/v0_6'/n)==orig[n],n
    l=read(p/UI)
    for token in ('SPEED_SCALE_SHORT 244U','SPEED_SCALE_LONG 219U',
                  'ASSIST_Y 101','SIGN_Y 8','UNIT_X 556',
                  'NAV_DISTANCE_MAX_WIDTH 120','static void refresh_acc(uint32_t now_ms)'):
        assert token in l, token
    assert 'twai_transmit(' not in read(p/'main/id/can/karoq_can_sniffer.c')
    assert 'twai_transmit(' not in read(p/'main/id/can/karoq_can_dispatcher.c')
    assert 'LISTEN_ONLY' in read(p/'main/id/can/karoq_can_sniffer.c')
    assert read(p/'main/ui/bridge/karoq_v06_live_bridge.c').count('lv_timer_create(tick, 100, NULL)')==1
    print('PASS: V1143 640x172 UI; 6x native V0.6 SHA256; NAV untouched; CAN LISTEN_ONLY; no extra UI timer')
    print('NOTE: complete ESP-IDF build and LCD test must run on Windows computer')
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--project',required=True,type=pathlib.Path)
    args=ap.parse_args()
    try:main(args.project.resolve())
    except Exception as e:print('FAIL:',repr(e),file=sys.stderr);sys.exit(1)
