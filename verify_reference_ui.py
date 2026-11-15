#!/usr/bin/env python3
"""Static verification of UI profile, original V0.6 integrity and receive-only path."""
import argparse,hashlib,json,pathlib,sys
UI=('karoq_hud.c','karoq_hud.h','karoq_hud_assets.c','karoq_hud_assets.h','karoq_hud_speed_digits.c','karoq_hud_speed_digits.h')
def main(p):
    def rd(rel):return (p/rel).read_text(encoding='utf-8-sig')
    manifest=json.loads(rd('docs/V113_UI_SHA256.json'))
    assert set(manifest)==set(UI)
    for f in UI:
        assert hashlib.sha256((p/'main/ui/v0_6'/f).read_bytes()).hexdigest()==manifest[f],f
    root=rd('CMakeLists.txt');assert 'project(karoq_can_hud_v1_0_13)' in root
    bridge=rd('main/ui/bridge/karoq_v06_live_bridge.c')
    assert bridge.count('KAROQ_REFERENCE_UI_V113')==1
    assert bridge.count('karoq_ref_layout_tick(')==1
    assert bridge.count('karoq_ref_layout_attach(')==1
    assert bridge.count('lv_timer_create(tick, 100, NULL)')==1
    assert 'karoq_ts_service_snapshot(now_ms)' in bridge
    dispatcher=rd('main/id/can/karoq_can_dispatcher.c')
    assert 'karoq_ui_acc_set_receive(' in dispatcher
    assert 'karoq_ts_vza_receive(' in dispatcher
    assert 'twai_transmit(' not in dispatcher
    can=rd('main/id/can/karoq_can_sniffer.c')
    assert 'LISTEN_ONLY' in can and 'twai_transmit(' not in can
    for filename in ['ui/layout/karoq_reference_assets.c','ui/layout/karoq_reference_layout.c','id/signals/karoq_ui_acc_set.c']:
        assert (p/'main'/filename).is_file(),filename
        assert '"'+filename+'"' in rd('main/CMakeLists.txt'),filename
    print('PASS: reference UI, V0.6 six source hashes, VZA/ACC/SD and CAN RX-only')
    print('NOTE: This is NOT an ESP-IDF compilation or physical-LCD test')
if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--project',required=True,type=pathlib.Path)
    x=a.parse_args()
    try:main(x.project.resolve())
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
