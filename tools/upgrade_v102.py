#!/usr/bin/env python3
"""Conservative V1.0.1 -> V1.0.2 migration, preserving byte-for-byte V0.6 HUD UI.
Usage: python tools/upgrade_v102.py --source PATH --dest PATH [--release PATH]
No third-party Python packages required. Never modifies the source project.
"""
import argparse
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path


def need(test, message):
    if not test:
        raise RuntimeError(message)


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def put(p, content):
    p.write_text(content, encoding='utf-8', newline='')


def run(source, dest, release):
    source, dest, release = (p.expanduser().resolve() for p in (source, dest, release))
    need(source.is_dir(), f'找不到已安裝的 V1.0.1 專案：{source}')
    need(not dest.exists(), f'目標資料夾已存在，拒絕覆蓋：{dest}')
    need(dest != source and source not in dest.parents and dest not in source.parents,
         '來源和目標路徑不可互相包含')
    srcmain = source / 'main'
    need((srcmain / 'core/main.c').is_file(), '來源不是已整合的 V1.0.1：缺少 main/core/main.c')
    need((srcmain / 'id/can/karoq_can_sniffer.c').is_file(), '缺少 CAN sniffer，停止更新')
    need((srcmain / 'id/decoders/karoq_can_speed.c').is_file(), '缺少車速解碼器')
    need((srcmain / 'id/decoders/karoq_can_acc_probe.c').is_file(), '缺少 ACC 解碼器')
    ui_src = srcmain / 'ui/legacy'
    if not (ui_src / 'karoq_hud.c').is_file():
        ui_src = srcmain / 'ui/v0_6'
    need((ui_src / 'karoq_hud.c').is_file(),
         '找不到真實 V0.6 介面：main/ui/legacy/karoq_hud.c。停止，不會生成假介面')
    need((ui_src / 'karoq_hud_assets.c').is_file(), '缺少真實 HUD assets，停止更新')
    need((srcmain / 'ui/bridge/karoq_hud_speed_bridge.c').is_file(), '缺少既有 V1.0.1 speed bridge')
    need((srcmain / 'ui/guard/karoq_hud_speed_guard.c').is_file(), '缺少暫時保留的 demo guard')
    need((source / 'CMakeLists.txt').is_file(), '缺少根目錄 CMakeLists.txt')
    need((release / 'main/CMakeLists.txt').is_file(), '升級包不完整，缺少 main/CMakeLists.txt')

    ui_hashes = {p.relative_to(ui_src).as_posix(): digest(p)
                 for p in ui_src.rglob('*') if p.is_file()}
    need(len(ui_hashes) >= 2, 'UI 資料夾不完整')
    temp = dest.parent / (dest.name + '__V102_TEMP')
    need(not temp.exists(), f'偵測到舊升級暫存資料夾，先檢查：{temp}')
    dest.parent.mkdir(parents=True, exist_ok=True)
    print('[1/6] 建立獨立 V1.0.2 專案，不修改既有 V1.0.1...')
    skip = {'build','.git','.venv','venv','history','__pycache__'}
    def ignore(_dir, names):
        return [n for n in names if n in skip or n.endswith('.pyc')]
    try:
        shutil.copytree(source, temp, ignore=ignore)
        main = temp / 'main'
        hist = temp / 'history/v1_0_1_backup'
        hist.mkdir(parents=True, exist_ok=True)
        for rel in ('core/main.c','CMakeLists.txt','ui/bridge/karoq_hud_speed_bridge.c',
                    'ui/guard/karoq_hud_speed_guard.c'):
            original = main / rel
            if original.is_file():
                backup = hist / rel
                backup.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(original, backup)
        print('[2/6] 將原分支 V0.6 UI 移入 main/ui/v0_6，保持原始位元內容...')
        v06 = main / 'ui/v0_6'
        old_legacy = main / 'ui/legacy'
        if old_legacy.exists():
            need(not v06.exists() or not any(v06.iterdir()), 'ui/legacy 與 ui/v0_6 都存在內容；停止以避免覆蓋')
            if v06.exists(): v06.rmdir()
            old_legacy.rename(v06)
        # No modifications to any V0.6 UI source or assets.
        for rel, expected in ui_hashes.items():
            need(digest(v06 / rel) == expected, f'V0.6 UI hash 不一致：{rel}')
        print(f'PASS: V0.6 UI 原始檔 {len(ui_hashes)} 個，SHA256 完全一致')
        (temp / 'docs').mkdir(parents=True,exist_ok=True)
        put(temp / 'docs/UI_V06_SHA256.json',json.dumps(ui_hashes,indent=2,ensure_ascii=False)+'\n')

        print('[3/6] 更新 CMake、版本資訊、車速橋接狀態字串...')
        shutil.copy2(release / 'main/CMakeLists.txt', main / 'CMakeLists.txt')
        shutil.copy2(release / 'main/core/karoq_firmware_info.h', main / 'core/karoq_firmware_info.h')
        shutil.copy2(release / 'main/ui/bridge/karoq_hud_speed_bridge.c',
                     main / 'ui/bridge/karoq_hud_speed_bridge.c')
        # CAN/ACC/guard code stays exactly as road-tested.
        core = main / 'core/main.c'
        content = core.read_text(encoding='utf-8-sig')
        need(content.count('karoq_hud_speed_bridge_attach();') == 1,
             'main.c 必須恰有一次 speed bridge attach；停止修改，請先確認版本')
        need(content.count('karoq_hud_ui_init();') == 1,
             'main.c 必須恰有一次原 UI 初始化；停止修改')
        need(content.count('karoq_can_sniffer_start()') == 1,
             'main.c 必須恰有一次 CAN 啟動；停止修改')
        need('karoq_hud_v08_ui_attach();' not in content,
             '偵測到 V0.8 overlay 啟動，停止修改')
        if '#include "karoq_firmware_info.h"' not in content:
            need('#include "karoq_hud_speed_bridge.h"' in content,
                 '找不到 speed bridge include，停止修改')
            content=content.replace('#include "karoq_hud_speed_bridge.h"',
                                    '#include "karoq_hud_speed_bridge.h"\n#include "karoq_firmware_info.h"',1)
        if 'KAROQ_FIRMWARE_VERSION' not in content:
            pattern=r'(?m)^([^\n]*karoq_hud_speed_bridge_attach\(\);[^\n]*)'
            msg=('\n        ESP_LOGI("KAROQ_CORE", '
                 '"FW v%s | UI=%s | SPEED=%s CAN RX | ACC=%s probe-only | guard=TEMP",\n'
                 '                 KAROQ_FIRMWARE_VERSION, KAROQ_UI_BASELINE,\n'
                 '                 KAROQ_CAN_SPEED_ID, KAROQ_ACC_PROBE_ID);')
            content, count = re.subn(pattern,lambda match:match.group(1)+msg,content,count=1)
            need(count==1,'無法找到 speed bridge 後的啟動紀錄插入點')
        put(core,content)
        root= temp / 'CMakeLists.txt'
        root_text=root.read_text(encoding='utf-8-sig')
        matches=list(re.finditer(r'\bproject\s*\(\s*([A-Za-z0-9_+.-]+)',root_text,re.I))
        need(len(matches)==1,'根目錄 project(...) 找不到或超過一個，停止改名')
        m=matches[0]
        root_text=root_text[:m.start(1)]+'karoq_can_hud_v1_0_2'+root_text[m.end(1):]
        put(root,root_text)

        print('[4/6] 更新文件、測試與版本，保留原 Guard 待移除 Demo writer...')
        for folder in ('docs','tests','tools'):
            target=temp/folder
            shutil.copytree(release/folder,target,dirs_exist_ok=True)
        for file in ('VERSION','CHANGELOG.md','README_安裝與驗證.md','Verify_V1_0_2.ps1'):
            shutil.copy2(release/file,temp/file)
        # preserve the actual UI fingerprints from local source after copying docs.
        put(temp/'docs/UI_V06_SHA256.json',json.dumps(ui_hashes,indent=2,ensure_ascii=False)+'\n')
        print('[5/6] 靜態一致性驗證...')
        cm=(main/'CMakeLists.txt').read_text(encoding='utf-8')
        need('CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$','',cm),
             'CMake 出現 CONFIGURE_DEPENDS')
        need('ui/v0_6' in cm and 'ui/legacy' not in cm, 'CMake UI 路徑不正確')
        need('"ui/guard/karoq_hud_speed_guard.c"' in cm,'Guard 在 demo writer 未移除前不能停用')
        need((main/'ui/v0_6/karoq_hud_assets.c').is_file(), '缺少 UI 素材')
        need('karoq_hud_speed_intercept.h' in (main/'ui/v0_6/karoq_hud.c').read_text(encoding='utf-8-sig'),
             '既有 V0.6 UI 未裝防覆寫 include；停止升級避免 88 閃現')
        need((main/'core/main.c').read_text().count('karoq_hud_speed_bridge_attach();')==1,
             '速度橋接建立次數異常')
        print('[6/6] 原子完成獨立專案...')
        temp.rename(dest)
        print('SUCCESS: V1.0.2 專案已建立：'+str(dest))
        print('CAN 與 ACC 保留實測原碼；UI V0.6 原始碼與資產位元未修改。')
        print('接著執行：idf.py reconfigure；idf.py build；idf.py -p COM8 flash monitor')
    except Exception:
        if temp.exists():
            shutil.rmtree(temp)
        raise

if __name__ == '__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--dest',type=Path,required=True)
    p.add_argument('--release',type=Path,default=Path(__file__).resolve().parents[1])
    a=p.parse_args()
    try: run(a.source,a.dest,a.release)
    except Exception as e:
        print('ERROR:',e,file=sys.stderr)
        sys.exit(1)
