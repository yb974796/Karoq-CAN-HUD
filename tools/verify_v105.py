#!/usr/bin/env python3
import argparse,hashlib,json,re,sys
from pathlib import Path

def check(root):
    main=root/'main';cm=(main/'CMakeLists.txt').read_text(encoding='utf-8')
    core=(main/'core/main.c').read_text(encoding='utf-8')
    sniffer=(main/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8')
    hashes=json.loads((root/'docs/V105_UI_SHA256.json').read_text(encoding='utf-8'))
    for name,h in hashes.items():
        assert hashlib.sha256((main/'ui/v0_6'/name).read_bytes()).hexdigest()==h, 'UI modified: '+name
    for name in ('id/signals/karoq_mqb_opendbc.c','other/diagnostics/karoq_mqb_openpilot_probe.c'):
        assert (main/name).is_file(), name
        assert '"'+name+'"' in cm, 'missing CMake source '+name
    assert 'CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$', '',cm)
    assert 'ui/v0_6' in cm and 'ui/legacy' not in cm
    assert core.count('karoq_can_sniffer_start()')==1
    assert core.count('karoq_mqb_openpilot_probe_init();')==1
    assert sniffer.count('karoq_sd_logger_receive(')==1
    assert sniffer.count('karoq_mqb_openpilot_probe_ingest(&msg);')==1
    assert sniffer.count('karoq_speed_ingest(&frame);')==1
    assert 'karoq_hud_v08_ui_attach();' not in core
    assert 'project(karoq_can_hud_v1_0_5)' in (root/'CMakeLists.txt').read_text()
    print('PASS: UI V0.6 unchanged',len(hashes),'files; all 3 hooks exactly once; V1.0.5 CMake valid')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--project',required=True,type=Path);a=p.parse_args()
    try:check(a.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
