#!/usr/bin/env python3
"""Read-only verification of an installed V1.0.8 firmware project."""
import argparse,hashlib,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def check(cond,msg):
    if not cond: raise RuntimeError(msg)

def verify(project):
    project=project.expanduser().resolve()
    main=project/'main'
    hdr=main/'core/main.c'
    ui=main/'ui/v0_6'
    source=(ROOT/'main/ui/v0_6') if (ROOT/'main/ui/v0_6').exists() else ui
    files=('karoq_hud.c','karoq_hud_assets.c','karoq_hud_speed_digits.c','karoq_hud_speed_digits.h',
           'karoq_hud.h','karoq_hud_assets.h')
    saved=json.loads((project/'docs/V108_UI_SHA256.json').read_text(encoding='utf-8'))
    for name in files:
        path=ui/name
        check(path.is_file(), 'MISSING real V0.6 UI: '+name)
        got=hashlib.sha256(path.read_bytes()).hexdigest()
        check(got==saved[name], 'REAL V0.6 UI modified unexpectedly: '+name)
    print('PASS: six V0.6 UI source/header files SHA256 match verified package')
    c=hdr.read_text(encoding='utf-8-sig')
    cm=(main/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    rootcm=(project/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    check('project(karoq_can_hud_v1_0_8)' in rootcm, 'wrong root project name')
    check(c.count('karoq_can_sniffer_start()')==1,'multiple/missing CAN starts')
    check(c.count('karoq_hud_ui_init();')==1,'multiple/missing HUD init')
    check(c.count('karoq_v06_live_bridge_attach();')==1,'missing/duplicate native HUD bridge')
    for term in ('karoq_hud_speed_bridge_attach();','karoq_v06_adas_ui_attach();','karoq_hud_v08_ui_attach();'):
        check(term not in c,'competing UI writer still active: '+term)
    for term in ('ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c',
                 'ui/v0_6/karoq_hud_speed_digits.c','ui/bridge/karoq_v06_live_bridge.c',
                 'ui/bridge/karoq_v06_live_model.c'):
        check(cm.count('"'+term+'"')==1,'source missing or duplicate: '+term)
    for term in ('"ui/bridge/karoq_hud_speed_bridge.c"','"ui/guard/karoq_hud_speed_guard.c"',
                 '"ui/bridge/karoq_v06_adas_ui_bridge.c"'):
        check(term not in cm,'competing bridge still built: '+term)
    check('KAROQ_HUD_DEMO_AUTORUN=0' in cm,'V0.6 Demo not explicitly disabled')
    check('CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$', '', cm), 'CMake not script-mode compatible')
    for term in ('karoq_sd_logger_init();','karoq_mqb_assist_runtime_init()','karoq_mqb_openpilot_probe_init()'):
        check(term in c,'missing initialized subsystem: '+term)
    sniffer=(main/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
    check('twai_transmit(' not in sniffer,'CAN transmit detected; not listen-only')
    # Check original status contract: no extra lane/road-sign/nav visual claims.
    original=(ui/'karoq_hud.c').read_text(encoding='utf-8-sig')
    check('lv_image_set_src' in original and 'karoq_hud_ui_set_state' in original,
          'Missing real V0.6 image-based speed update API')
    print('PASS: 1 native V0.6 display writer, Demo OFF, no Label bridge/Guard/overlay')
    print('PASS: CAN, SD and MQB modules still connected, no TX call in CAN source')
    print('NOTE: This is a SOURCE verifier, not ESP-IDF complete build or actual car test')
if __name__=='__main__':
    cli=argparse.ArgumentParser();cli.add_argument('--project',required=True,type=Path);a=cli.parse_args()
    try:verify(a.project)
    except Exception as e:print('FAIL:',e,file=sys.stderr);sys.exit(1)
