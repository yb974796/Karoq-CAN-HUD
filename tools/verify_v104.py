#!/usr/bin/env python3
import argparse,hashlib,json,re,sys
from pathlib import Path

def check(root):
    main=root/'main'
    cm=(main/'CMakeLists.txt').read_text(encoding='utf-8')
    core=(main/'core/main.c').read_text(encoding='utf-8')
    sniff=(main/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8')
    logger=(main/'other/storage/karoq_sd_logger.c').read_text(encoding='utf-8')
    button=(main/'other/buttons/karoq_record_button.c').read_text(encoding='utf-8')
    fingerprint=json.loads((root/'docs/V104_UI_INTEGRITY.json').read_text(encoding='utf-8'))
    assert len(fingerprint)>=2,'Missing original V0.6 assets'
    for name,expected in fingerprint.items():
        assert hashlib.sha256((main/'ui/v0_6'/name).read_bytes()).hexdigest()==expected, 'UI changed: '+name
    assert core.count('karoq_sd_logger_init();')==1,'Logger init count incorrect'
    assert sniff.count('karoq_sd_logger_receive(')==1,'RX receive wrapper count incorrect'
    assert not re.search(r'\btwai_receive\s*\(',sniff),'Sniffer must only use logger wrapper'
    assert len(re.findall(r'"other/(storage|buttons)/karoq_\w+\.c"',cm))==3
    assert 'CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$','',cm)
    assert 'ui/v0_6' in cm and 'ui/legacy' not in cm
    assert 'sdmmc' in cm and 'fatfs' in cm
    assert 'project(karoq_can_hud_v1_0_4)' in (root/'CMakeLists.txt').read_text()
    assert 'atomic_fetch_add_explicit(&s_rx_inflight' in logger
    assert 'while(atomic_load_explicit(&s_rx_inflight' in logger
    assert 'record_cmd_t cmd=CMD_TOGGLE' in logger
    assert 'queue_peak' in logger and 'META.txt' in logger
    assert '#define BUTTON_EVENT_COOLDOWN_MS 300' in button
    print('PASS: V1.0.4 migration, all original UI hashes, receive integration, guarded queue drain')
    print('NOTE: static checks; full ESP-IDF build and SDMMC hardware test must be performed on device')

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--project',type=Path,required=True)
    x=a.parse_args()
    try:check(x.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
