#!/usr/bin/env python3
from pathlib import Path
import argparse,sys,json,hashlib,re

def check(proj):
    main=proj/'main'
    cm=(main/'CMakeLists.txt').read_text(encoding='utf-8')
    core=(main/'core/main.c').read_text(encoding='utf-8')
    sniffer=(main/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8')
    manifest=json.loads((proj/'docs/V103_UI_INTEGRITY.json').read_text(encoding='utf-8'))
    for rel,hash0 in manifest.items():
        assert hashlib.sha256((main/'ui/v0_6'/rel).read_bytes()).hexdigest()==hash0, 'UI changed: '+rel
    assert core.count('karoq_sd_logger_init();')==1
    assert sniffer.count('karoq_sd_logger_receive(')==1
    assert not re.search(r'(?<!logger_)\btwai_receive\s*\(',sniffer)
    assert len(re.findall(r'"other/(storage|buttons)/karoq_\w+\.c"',cm))==3 # logger, tracker, button
    assert 'fatfs' in cm and 'esp_driver_sdmmc' in cm and 'sdmmc' in cm
    assert 'CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$','',cm)
    assert 'ui/v0_6' in cm
    assert 'karoq_can_hud_v1_0_3' in (proj/'CMakeLists.txt').read_text()
    print('PASS: V1.0.3 RX hook + button + SDMMC sources + unchanged V0.6 UI ('+str(len(manifest))+')')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--project',type=Path,required=True);a=p.parse_args()
    try:check(a.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
