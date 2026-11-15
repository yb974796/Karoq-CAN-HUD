#!/usr/bin/env python3
"""V1.0.4 -> V1.0.5, fail-closed and source-preserving. Standard library only."""
import argparse,hashlib,json,re,shutil,sys
from pathlib import Path

PATCH_FILES=(
 'id/signals/karoq_mqb_opendbc.h',
 'id/signals/karoq_mqb_opendbc.c',
 'other/diagnostics/karoq_mqb_openpilot_probe.h',
 'other/diagnostics/karoq_mqb_openpilot_probe.c',
)

def require(x,why):
    if not x: raise RuntimeError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def update(source,dest,package):
    source,dest,package=(p.expanduser().resolve() for p in (source,dest,package))
    require(source.is_dir(),'V1.0.4 來源目錄不存在: '+str(source))
    require(not dest.exists(),'目標目錄已存在，拒絕覆蓋: '+str(dest))
    require(source!=dest and source not in dest.parents and dest not in source.parents,
            '來源與目標必須是互不包含的獨立目錄')
    base=source/'main'
    reqs=['core/main.c','id/can/karoq_can_sniffer.c','id/decoders/karoq_can_speed.c',
          'id/decoders/karoq_can_acc_probe.c','ui/v0_6/karoq_hud.c',
          'ui/v0_6/karoq_hud_assets.c','other/storage/karoq_sd_logger.c',
          'other/buttons/karoq_record_button.c','CMakeLists.txt']
    for x in reqs:require((base/x).is_file(),'來源缺少必要檔案: main/'+x)
    ui=base/'ui/v0_6'
    fingerprints={p.relative_to(ui).as_posix():sha(p) for p in ui.rglob('*') if p.is_file()}
    require(len(fingerprints)>=2,'缺少完整 V0.6 UI')
    core=(base/'core/main.c').read_text(encoding='utf-8-sig')
    sniffer=(base/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
    cmake=(base/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    root=(source/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    require(core.count('karoq_sd_logger_init();')==1,'來源尚未整合 V1.0.4 SD Logger')
    require(sniffer.count('karoq_sd_logger_receive(')==1,'來源 CAN RX hook 非預期')
    require(sniffer.count('karoq_speed_ingest(&frame);')==1,
            '無法定位原車速解碼；停止，避免誤改 CAN 接收迴圈')
    require('karoq_mqb_openpilot_probe' not in core+sniffer+cmake,'已安裝新版 MQB 探針，拒絕重複')
    require(len(re.findall(r'\bkaroq_can_sniffer_start\s*\(\s*\)',core))==1,
            'CAN 啟動必須恰好一次')
    require('"id/can/karoq_can_sniffer.c"' in cmake and 'INCLUDE_DIRS' in cmake,
            'CMake 沒有預期的 SRCS/INCLUDE_DIRS 錨點')
    require('ui/v0_6' in cmake and 'ui/legacy' not in cmake,'不是 V0.6 UI')
    require('CONFIGURE_DEPENDS' not in re.sub(r'(?m)^\s*#.*$','',cmake),
            'CMake 含不相容 CONFIGURE_DEPENDS')
    require(re.search(r'\bproject\s*\(\s*karoq_can_hud_v1_0_4\b',root) is not None,
            '來源根目錄 project() 版本不是 V1.0.4')
    tmp=dest.parent/(dest.name+'__V105_TEMP')
    require(not tmp.exists(),'存在未處理的升級暫存資料夾: '+str(tmp))
    dest.parent.mkdir(parents=True,exist_ok=True)
    try:
        shutil.copytree(source,tmp,ignore=lambda _,names:
            [n for n in names if n in {'build','.git','.venv','venv','__pycache__'} or n.endswith('.pyc')])
        tmain=tmp/'main';hist=tmp/'history/V1_0_4_Openpilot_Upgrade'
        for x in ('core/main.c','id/can/karoq_can_sniffer.c','CMakeLists.txt'):
            target=hist/x;target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(tmain/x,target)
        for file in PATCH_FILES:
            p=tmain/file;p.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(package/'main'/file,p)
        # No global CAN receive replacement, no extra TWAI driver, no UI modifications.
        sniffer=sniffer.replace('#include "karoq_can_sniffer.h"',
           '#include "karoq_can_sniffer.h"\n#include "karoq_mqb_openpilot_probe.h"',1)
        require('#include "karoq_mqb_openpilot_probe.h"' in sniffer,
                '無法加入 RX 診斷 include，停止')
        sniffer=sniffer.replace('karoq_speed_ingest(&frame);',
           'karoq_speed_ingest(&frame);\n            karoq_mqb_openpilot_probe_ingest(&msg);',1)
        (tmain/'id/can/karoq_can_sniffer.c').write_text(sniffer,encoding='utf-8')
        core='#include "karoq_mqb_openpilot_probe.h"\n'+core
        m=re.search(r'(?m)^([ \t]*)([^\n]*\bkaroq_can_sniffer_start\s*\(\s*\)[^\n]*;)',core)
        require(m is not None,'無法尋找 CAN 啟動程式碼')
        ind=m.group(1)
        addition=(ind+'esp_err_t mqb_diag_status = karoq_mqb_openpilot_probe_init();\n'
          +ind+'if (mqb_diag_status != ESP_OK) {\n'
          +ind+'    ESP_LOGW("KAROQ_OP_MQB", "MQB probe init failed: %d; CAN continues", (int)mqb_diag_status);\n'
          +ind+'}\n')
        core=core[:m.start()]+addition+core[m.start():]
        (tmain/'core/main.c').write_text(core,encoding='utf-8')
        cmake=cmake.replace('"id/can/karoq_can_sniffer.c"',
          '"id/can/karoq_can_sniffer.c"\n        "id/signals/karoq_mqb_opendbc.c"\n        "other/diagnostics/karoq_mqb_openpilot_probe.c"',1)
        cmake=cmake.replace('INCLUDE_DIRS',
          'INCLUDE_DIRS\n        "id/signals"\n        "other/diagnostics"',1)
        (tmain/'CMakeLists.txt').write_text(cmake,encoding='utf-8')
        root=re.sub(r'\bproject\s*\(\s*karoq_can_hud_v1_0_4\b',
          'project(karoq_can_hud_v1_0_5',root,count=1)
        (tmp/'CMakeLists.txt').write_text(root,encoding='utf-8')
        (tmp/'VERSION').write_text('1.0.5\nUI=CAN_SNIFFER_V0.6\nCAN=LISTEN_ONLY\nMQB_PROBE=READ_ONLY\n',encoding='utf-8')
        for name in ('README_安裝.md','CHANGELOG.md'):
            shutil.copy2(package/name,tmp/name)
        for folder in ('tests','tools','docs'):
            shutil.copytree(package/folder,tmp/folder,dirs_exist_ok=True)
        (tmp/'docs/V105_UI_SHA256.json').write_text(
          json.dumps(fingerprints,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
        require(all(sha(tmain/'ui/v0_6'/name)==h for name,h in fingerprints.items()),
                'V0.6 UI 位元發生變化，拒絕升級')
        for x in ('id/decoders/karoq_can_speed.c','id/decoders/karoq_can_acc_probe.c',
                  'other/storage/karoq_sd_logger.c','other/buttons/karoq_record_button.c'):
            require(sha(base/x)==sha(tmain/x),'既有經測試模組被修改: '+x)
        require(sniffer.count('karoq_sd_logger_receive(')==1 and
                sniffer.count('karoq_mqb_openpilot_probe_ingest(&msg);')==1,
                'MQB RX 診斷整合出錯')
        require(core.count('karoq_mqb_openpilot_probe_init();')==1,'MQB 初始化次數錯誤')
        tmp.rename(dest)
        print('SUCCESS: V1.0.5 independent project created:',dest)
        print('V0.6 UI unchanged SHA256:',len(fingerprints),'files')
        print('Speed/ACC/SD/BOOT unchanged; new read-only MQB metadata+probe installed.')
    except Exception:
        if tmp.exists():shutil.rmtree(tmp)
        raise
if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--source',required=True,type=Path)
    p.add_argument('--dest',required=True,type=Path)
    p.add_argument('--addon',default=Path(__file__).resolve().parents[1],type=Path)
    a=p.parse_args()
    try:update(a.source,a.dest,a.addon)
    except Exception as e:print('ERROR:',e,file=sys.stderr);sys.exit(1)
