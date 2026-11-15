#!/usr/bin/env python3
import argparse,hashlib,json,re,sys
from pathlib import Path

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def verify(root):
    main=root/'main'
    must=['core/main.c','core/karoq_firmware_info.h','id/can/karoq_can_sniffer.c',
          'id/decoders/karoq_can_speed.c','id/decoders/karoq_can_acc_probe.c',
          'ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c',
          'ui/bridge/karoq_hud_speed_bridge.c','ui/guard/karoq_hud_speed_guard.c',
          'CMakeLists.txt']
    for rel in must:
        assert (main/rel).is_file(), 'MISSING: '+rel
        print('PASS:',rel)
    fingerprints=json.loads((root/'docs/UI_V06_SHA256.json').read_text(encoding='utf-8'))
    for rel,expected in fingerprints.items():
        assert sha(main/'ui/v0_6'/rel)==expected, 'V0.6 UI file changed: '+rel
    print('PASS: V0.6 原 UI 與資產 SHA256 全數一致 ('+str(len(fingerprints))+')')
    core=(main/'core/main.c').read_text(encoding='utf-8')
    assert core.count('karoq_hud_ui_init();')==1
    assert core.count('karoq_hud_speed_bridge_attach();')==1
    assert core.count('karoq_can_sniffer_start()')==1
    assert 'karoq_hud_v08_ui_attach();' not in core
    cm=(main/'CMakeLists.txt').read_text()
    no_comment=re.sub(r'(?m)^\s*#.*$', '', cm)
    assert 'CONFIGURE_DEPENDS' not in no_comment
    assert 'ui/v0_6' in cm and 'ui/legacy' not in cm
    assert '"ui/guard/karoq_hud_speed_guard.c"' in cm
    root_cmake=(root/'CMakeLists.txt').read_text()
    assert 'project(karoq_can_hud_v1_0_2)' in root_cmake
    assert 'KAROQ_FIRMWARE_VERSION' in core
    print('PASS: 1 CAN + 1 V0.6 HUD + 1 speed bridge, no V0.8 overlay')
    print('PASS: version V1.0.2; ESP-IDF 5.5.3 CMake Script Mode compatible')
    print('NOTE: Guard remains temporary until demo writer removed from original V0.6 source')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--project',type=Path,required=True);a=p.parse_args()
    try:verify(a.project)
    except Exception as e:
        print('FAIL:',e,file=sys.stderr);sys.exit(1)
