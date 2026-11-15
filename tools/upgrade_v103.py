#!/usr/bin/env python3
"""V1.0.2 -> V1.0.3 additive patch. Source never modified. Fail closed."""
from pathlib import Path
import argparse,hashlib,shutil,re,sys,json

FILES=[
  'other/storage/karoq_sd_logger.c','other/storage/karoq_sd_logger.h',
  'other/storage/karoq_id_tracker.c','other/storage/karoq_id_tracker.h',
  'other/buttons/karoq_record_button.c','other/buttons/karoq_record_button.h',
]

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def need(v,msg):
    if not v:raise RuntimeError(msg)

def patch(src,dest,addon):
    src=src.expanduser().resolve();dest=dest.expanduser().resolve();addon=addon.resolve()
    need(src.is_dir(),'Source V1.0.2 project not found: '+str(src))
    need(not dest.exists(),'Destination already exists; refused to overwrite: '+str(dest))
    need(src!=dest and src not in dest.parents and dest not in src.parents,'Source/destination must be independent')
    main=src/'main'
    essentials=['core/main.c','id/can/karoq_can_sniffer.c','ui/v0_6/karoq_hud.c',
        'ui/v0_6/karoq_hud_assets.c','ui/bridge/karoq_hud_speed_bridge.c','CMakeLists.txt']
    for n in essentials: need((main/n).is_file(),'Cannot locate V1.0.2 file: main/'+n)
    core=(main/'core/main.c').read_text(encoding='utf-8-sig')
    sniffer=(main/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
    cmake=(main/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    root=(src/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    need(re.search(r'\bproject\s*\(',root) is not None,'Missing ESP-IDF root project()')
    need(len(re.findall(r'\btwai_receive\s*\(',sniffer))==1,'Need exactly one twai_receive() in CAN sniffer; stopped')
    need(len(re.findall(r'\bkaroq_can_sniffer_start\s*\(',core))==1,'Need exactly one CAN startup call; stopped')
    need('karoq_sd_logger' not in core+sniffer+cmake,'Logger appears to be already installed; stopped')
    need('"id/can/karoq_can_sniffer.c"' in cmake,'Cannot locate CMake SRCS insertion anchor')
    need('PRIV_REQUIRES' in cmake and 'INCLUDE_DIRS' in cmake,'CMake requirement block not found')
    need('ui/v0_6' in cmake,'Required original V0.6 UI not detected')
    need('CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$', '', cmake),'Broken CMake glob config')
    # Check the existing LCD pin config. Official V2 LCD uses SPI3 and SD uses SDMMC 1-bit.
    for path in (src/'components',main/'other/project_legacy',src/'main'):
        if path.exists():
            for cfg in path.rglob('user_config.h'):
                text=cfg.read_text(encoding='utf-8-sig',errors='ignore')
                if 'LCD_HOST' in text:
                    need(re.search(r'#\s*define\s+LCD_HOST\s+SPI3_HOST',text) is not None,
                         'LCD_HOST is not SPI3_HOST in '+str(cfg)+', check board pinout before enabling SD')
    fingerprints={p.relative_to(main/'ui/v0_6').as_posix():sha(p)
        for p in (main/'ui/v0_6').rglob('*') if p.is_file()}

    temp=dest.parent/(dest.name+'__V103_TMP')
    need(not temp.exists(),'Temporary update directory already exists; inspect first: '+str(temp))
    dest.parent.mkdir(parents=True,exist_ok=True)
    try:
        shutil.copytree(src,temp,ignore=lambda _,names:[x for x in names if x in ('build','.git','.venv','venv','__pycache__') or x.endswith('.pyc')])
        target=temp/'main'
        for n in FILES:
            p=target/n;p.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(addon/'main'/n,p)
        sniffer=(target/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
        sniffer=re.sub(r'\btwai_receive\s*\(', 'karoq_sd_logger_receive(',sniffer,count=1)
        sniffer='#include "karoq_sd_logger.h"\n'+sniffer
        (target/'id/can/karoq_can_sniffer.c').write_text(sniffer,encoding='utf-8')
        core=(target/'core/main.c').read_text(encoding='utf-8-sig')
        # Keep CAN initialization exactly where it was, but add logger init before it.
        m=re.search(r'(?m)^([ \t]*)([^\n]*\bkaroq_can_sniffer_start\s*\(\s*\)[^\n]*;)',core)
        need(m is not None,'Unable to identify CAN start statement in core/main.c')
        indent=m.group(1)
        registration=(indent+'esp_err_t karoq_logger_status = karoq_sd_logger_init();\n'
            +indent+'if (karoq_logger_status != ESP_OK) {\n'
            +indent+'    ESP_LOGW("KAROQ_SD", "Logger init failed: %s (CAN continues)", esp_err_to_name(karoq_logger_status));\n'
            +indent+'}\n')
        core=core[:m.start()]+registration+core[m.start():]
        core='#include "karoq_sd_logger.h"\n'+core
        (target/'core/main.c').write_text(core,encoding='utf-8')
        cmake=(target/'CMakeLists.txt').read_text(encoding='utf-8-sig')
        added='\n'.join('        "'+n+'"' for n in FILES)
        cmake=cmake.replace('"id/can/karoq_can_sniffer.c"',
            '"id/can/karoq_can_sniffer.c"\n'+added,1)
        cmake=cmake.replace('INCLUDE_DIRS','INCLUDE_DIRS\n        "other/storage"\n        "other/buttons"',1)
        cmake=cmake.replace('PRIV_REQUIRES','PRIV_REQUIRES\n        esp_driver_gpio\n        esp_driver_sdmmc\n        sdmmc\n        fatfs',1)
        (target/'CMakeLists.txt').write_text(cmake,encoding='utf-8')
        root=(temp/'CMakeLists.txt').read_text(encoding='utf-8-sig')
        root=re.sub(r'\bproject\s*\(\s*\w+', 'project(karoq_can_hud_v1_0_3',root,count=1)
        (temp/'CMakeLists.txt').write_text(root,encoding='utf-8')
        (temp/'VERSION').write_text('1.0.3\nUI=CAN_SNIFFER_V0.6\nSD=SDMMC_1BIT\nBUTTON=BOOT_GPIO0\n',encoding='utf-8')
        for n in ('README_安裝.md','CHANGELOG.md'):
            shutil.copy2(addon/n,temp/n)
        (temp/'docs').mkdir(exist_ok=True)
        (temp/'docs/V103_UI_INTEGRITY.json').write_text(json.dumps(fingerprints,indent=2,ensure_ascii=False),encoding='utf-8')
        # Copy tester and utility scripts.
        shutil.copytree(addon/'tests',temp/'tests',dirs_exist_ok=True)
        shutil.copytree(addon/'tools',temp/'tools',dirs_exist_ok=True)
        for rel,digest in fingerprints.items():
            need(sha(target/'ui/v0_6'/rel)==digest,'UI file changed by update: '+rel)
        need((target/'id/can/karoq_can_sniffer.c').read_text().count('karoq_sd_logger_receive(')==1,'RX hook missing')
        need((target/'core/main.c').read_text().count('karoq_sd_logger_init();')==1,'Logger startup missing')
        temp.rename(dest)
        print('SUCCESS: V1.0.3 created:',dest)
        print('V0.6 UI/graphics unchanged:',len(fingerprints),'files SHA-256 verified.')
        print('CAN Listen Only unchanged; SD/BOOT recorder added.')
    except Exception:
        if temp.exists():shutil.rmtree(temp)
        raise

if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('--source',type=Path,required=True)
    ap.add_argument('--dest',type=Path,required=True)
    ap.add_argument('--addon',type=Path,default=Path(__file__).resolve().parents[1])
    a=ap.parse_args()
    try:patch(a.source,a.dest,a.addon)
    except Exception as e:print('ERROR:',e,file=sys.stderr);sys.exit(1)
