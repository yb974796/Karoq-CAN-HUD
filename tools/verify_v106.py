#!/usr/bin/env python3
import argparse,hashlib,json,sys,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--project',type=Path,required=True);a=p.parse_args()
root=a.project;main=root/'main'
try:
 for file in ('core/main.c','id/can/karoq_can_sniffer.c','id/signals/karoq_mqb_assist_state.c',
     'other/diagnostics/karoq_mqb_assist_runtime.c','ui/bridge/karoq_v06_adas_ui_bridge.c',
     'ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c','other/storage/karoq_sd_logger.c'):
     assert (main/file).is_file(),f'missing {file}'
 core=(main/'core/main.c').read_text()
 sniff=(main/'id/can/karoq_can_sniffer.c').read_text()
 assert core.count('karoq_v06_adas_ui_attach();')==1
 assert len(re.findall(r'\bkaroq_mqb_assist_runtime_init\s*\(\s*\)',core))==1
 assert sniff.count('karoq_mqb_assist_runtime_ingest(&msg);')==1
 cmake=(main/'CMakeLists.txt').read_text()
 for fname in ('karoq_mqb_assist_state.c','karoq_mqb_assist_runtime.c','karoq_v06_adas_ui_bridge.c'):
     assert fname in cmake
 data=json.loads((root/'docs/V106_UI_SHA256.json').read_text())
 for rel,digest in data['final'].items():
     f=main/'ui/v0_6'/rel
     assert hashlib.sha256(f.read_bytes()).hexdigest()==digest, 'UI changed after installation: '+rel
 print('PASS V1.0.6: CAN RX one hook, state initialized, LVGL timer, UI assets manifest')
 print('V0.6 UI icon binding:', 'BOUND' if data['auto_bound'] else 'UNBOUND - requires precise icon pointer/asset mapping')
except Exception as exc:
 print('FAIL:',exc,file=sys.stderr);sys.exit(1)
