#!/usr/bin/env python3
"""V1.0.3 -> V1.0.4 independent, fail-closed upgrade; preserve V0.6 UI bytes."""
from pathlib import Path
import argparse, hashlib, json, shutil, re, sys

FILES = ['other/storage/karoq_sd_logger.c',
         'other/storage/karoq_sd_logger.h',
         'other/storage/karoq_id_tracker.c',
         'other/storage/karoq_id_tracker.h',
         'other/buttons/karoq_record_button.c',
         'other/buttons/karoq_record_button.h']


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def need(condition, msg):
    if not condition: raise RuntimeError(msg)


def patch(src, dst, addon):
    src, dst, addon = (p.expanduser().resolve() for p in (src, dst, addon))
    need(src.is_dir(), 'Cannot locate installed V1.0.3 project: '+str(src))
    need(not dst.exists(), 'Destination exists. Refusing to overwrite: '+str(dst))
    need(src != dst and src not in dst.parents and dst not in src.parents,
         'Source and destination must be separate folders')
    main=src/'main'
    needed=['core/main.c','id/can/karoq_can_sniffer.c',
            'ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c',
            'ui/bridge/karoq_hud_speed_bridge.c','CMakeLists.txt'] + FILES
    for file in needed: need((main/file).is_file(), 'Missing V1.0.3 source: main/'+file)
    core=(main/'core/main.c').read_text(encoding='utf-8-sig')
    sniffer=(main/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
    cmake=(main/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    need(core.count('karoq_sd_logger_init();')==1, 'Missing or duplicate SD logger init')
    need(sniffer.count('karoq_sd_logger_receive(')==1, 'Missing or duplicate CAN RX logging hook')
    need('"other/storage/karoq_sd_logger.c"' in cmake, 'No logger source registered in CMake')
    need('"other/buttons/karoq_record_button.c"' in cmake, 'No button source registered in CMake')
    need('ui/v0_6' in cmake, 'Missing required V0.6 UI in CMake')
    need('CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$', '', cmake),
         'CMake still has Script Mode incompatible CONFIGURE_DEPENDS')
    root_cm=(src/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    need(re.search(r'project\s*\(\s*karoq_can_hud_v1_0_3\b',root_cm) is not None,
         'Source project name should be karoq_can_hud_v1_0_3')
    ui=main/'ui/v0_6'
    hashes={p.relative_to(ui).as_posix():sha(p) for p in ui.rglob('*') if p.is_file()}
    need(len(hashes)>=2, 'V0.6 UI source/assets missing')
    temp=dst.parent/(dst.name+'__V104_TEMP')
    need(not temp.exists(), 'Leftover temp folder: '+str(temp))
    dst.parent.mkdir(parents=True,exist_ok=True)
    try:
        shutil.copytree(src,temp,ignore=lambda _,ns:[x for x in ns if x in
            ('build','.git','venv','.venv','__pycache__') or x.endswith('.pyc')])
        target=temp/'main'
        hist=temp/'history/V1_0_3_Logger_Backup'
        for file in FILES:
            p=target/file
            old=hist/file
            old.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(p,old)
            shutil.copy2(addon/'main'/file,p)
        root= temp/'CMakeLists.txt'
        t=root.read_text(encoding='utf-8-sig')
        t,n=re.subn(r'\bproject\s*\(\s*karoq_can_hud_v1_0_3\b',
            'project(karoq_can_hud_v1_0_4',t,count=1)
        need(n==1, 'Cannot update project()')
        root.write_text(t,encoding='utf-8')
        for name in ('README_安裝.md','CHANGELOG.md'):
            shutil.copy2(addon/name,temp/name)
        (temp/'VERSION').write_text('1.0.4\nUI=CAN_SNIFFER_V0.6\nSD=SDMMC_1BIT\nBUTTON=BOOT_GPIO0\n',encoding='utf-8')
        shutil.copytree(addon/'tests',temp/'tests',dirs_exist_ok=True)
        shutil.copytree(addon/'tools',temp/'tools',dirs_exist_ok=True)
        fp=temp/'docs/V104_UI_INTEGRITY.json'
        fp.parent.mkdir(parents=True,exist_ok=True)
        fp.write_text(json.dumps(hashes,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        for file,h in hashes.items():
            need(sha(target/'ui/v0_6'/file)==h,'UI checksum mismatch: '+file)
        need((target/'core/main.c').read_text(encoding='utf-8-sig')==core,
             'Core main.c changed unexpectedly')
        need((target/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')==sniffer,
             'CAN sniffer changed unexpectedly')
        temp.rename(dst)
        print('SUCCESS: independent V1.0.4 project:',dst)
        print('PASS: V0.6 UI SHA-256 unchanged:',len(hashes),'files')
        print('PASS: core, CAN RX, speed decoder and ACC decoder unchanged')
    except Exception:
        if temp.exists(): shutil.rmtree(temp)
        raise

if __name__=='__main__':
    a=argparse.ArgumentParser()
    a.add_argument('--source',type=Path,required=True)
    a.add_argument('--dest',type=Path,required=True)
    a.add_argument('--addon',type=Path,default=Path(__file__).resolve().parents[1])
    x=a.parse_args()
    try:patch(x.source,x.dest,x.addon)
    except Exception as e:print('ERROR:',e,file=sys.stderr);sys.exit(1)
