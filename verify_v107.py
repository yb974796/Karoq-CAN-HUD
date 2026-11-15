#!/usr/bin/env python3
import argparse,hashlib,json,re,sys
from pathlib import Path

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def verify(root):
    main=root/'main'
    assert 'project(karoq_can_hud_v1_0_7' in (root/'CMakeLists.txt').read_text()
    fw=(main/'core/karoq_firmware_info.h').read_text()
    assert '#define KAROQ_FIRMWARE_VERSION "1.0.7"' in fw
    must=['core/main.c','id/can/karoq_can_sniffer.c','id/decoders/karoq_can_speed.c',
        'id/decoders/karoq_can_acc_probe.c','id/signals/karoq_mqb_opendbc.c',
        'id/signals/karoq_mqb_assist_state.c','other/storage/karoq_sd_logger.c',
        'other/buttons/karoq_record_button.c','other/diagnostics/karoq_mqb_openpilot_probe.c',
        'other/diagnostics/karoq_mqb_assist_runtime.c','ui/bridge/karoq_hud_speed_bridge.c',
        'ui/bridge/karoq_v06_adas_ui_bridge.c','ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c']
    for rel in must:assert (main/rel).is_file(), 'missing '+rel
    cm=(main/'CMakeLists.txt').read_text()
    for rel in must:
        if rel.endswith('.c') and rel not in ('ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c'):
            assert '"'+rel+'"' in cm, 'not in CMake SRCS: '+rel
    assert 'ui/v0_6' in cm and 'CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$', '', cm)
    assert 'karoq_hud_v08_ui.c' not in cm
    src=(main/'id/can/karoq_can_sniffer.c').read_text()
    core=(main/'core/main.c').read_text()
    for f in ('karoq_sd_logger_receive(', 'karoq_mqb_openpilot_probe_ingest(&msg);',
              'karoq_mqb_assist_runtime_ingest(&msg);'):
        assert src.count(f)==1, 'single RX hook: '+f
    for f in ('karoq_can_sniffer_start()', 'karoq_sd_logger_init();',
              'karoq_mqb_openpilot_probe_init();',
              'karoq_hud_ui_init();','karoq_hud_speed_bridge_attach();','karoq_v06_adas_ui_attach();'):
        assert core.count(f)==1, 'single init: '+f
    assert len(re.findall(r'\bkaroq_mqb_assist_runtime_init\s*\(\s*\)',core))==1
    assert 'twai_transmit(' not in src
    assert 'karoq_hud_v08_ui_attach();' not in core
    fps=json.loads((root/'docs/V107_UI_SHA256.json').read_text(encoding='utf-8'))
    assert fps
    for rel,h in fps.items(): assert sha(main/'ui/v0_6'/rel)==h, 'changed original V0.6 UI: '+rel
    assert (root/'docs/V106_LINK_STATUS.txt').exists(),'missing V0.6 binding status'
    print('PASS V1.0.7: all modules + unique CAN/LVGL startup + original V0.6 UI SHA256 + safe CMake')
    print((root/'docs/V106_LINK_STATUS.txt').read_text(encoding='utf-8').splitlines()[0])
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--project',required=True,type=Path)
    a=p.parse_args()
    try:verify(a.project)
    except Exception as e:print('FAIL V1.0.7:',e,file=sys.stderr);sys.exit(1)
