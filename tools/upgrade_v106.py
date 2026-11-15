#!/usr/bin/env python3
"""V1.0.5 -> V1.0.6. Preserves private V0.6 HUD byte-for-byte unless a
unique original assist image/asset mapping can be safely identified.
"""
import argparse, hashlib, json, re, shutil, sys
from pathlib import Path
PKG = Path(__file__).resolve().parents[1]
PATCH_FILES=[
 'id/signals/karoq_mqb_assist_state.h','id/signals/karoq_mqb_assist_state.c',
 'other/diagnostics/karoq_mqb_assist_runtime.h','other/diagnostics/karoq_mqb_assist_runtime.c',
 'ui/bridge/karoq_v06_adas_ui_bridge.h','ui/bridge/karoq_v06_adas_ui_bridge.c'
]

def check(cond,why):
    if not cond: raise RuntimeError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def patch_icon_binding(ui_source):
    """Only auto-patch if existing source uses one identifiable image object and
    four explicit assets with the expected names. Other conventions are left untouched.
    """
    text=ui_source.read_text(encoding='utf-8-sig')
    # Exact symbols; no automated guess for images rendered via canvas/style/tint.
    keys=['assist_off','assist_acc','assist_lane','assist_travel']
    if not all(re.search(r'\b'+k+r'\b',text) for k in keys):return False,'缺少四個具名的 V0.6 image 資產；不猜圖示'
    candidates=re.findall(r'lv_image_set_src\s*\(\s*([A-Za-z_]\w*)\s*,\s*&?assist_off\s*\)',text)
    if len(set(candidates))!=1:return False,'無法唯一辨識原 LVGL 輔助圖片物件'
    obj=candidates[0]
    fn=re.search(r'\bvoid\s+karoq_hud_ui_init\s*\([^)]*\)\s*\{',text)
    if not fn:return False,'找不到 V0.6 UI 初始化函式'
    # Ensure variable name is local to the same initializer or file-level scope;
    # we require it in the init body to avoid binding an undeclared variable.
    start=fn.end();depth=1;end=start
    while end < len(text) and depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    if depth:return False,'UI 初始化函式括號不完整'
    body=text[start:end-1]
    if not re.search(r'\b'+re.escape(obj)+r'\b',body):return False,'圖片物件不在 V0.6 初始化函式作用域內'
    if not re.search(r'\blv_obj_t\s*\*\s*'+re.escape(obj)+r'\b',body):
        return False,'圖片物件非可識別的局部 lv_obj_t*；拒絕盲目使用'
    # For every asset, require that name is already a source argument or declared in this file.
    # An exact source literal is reused; does not invent a symbol type.
    binding='\n    karoq_v06_adas_bind_image('+obj+', &assist_off, &assist_acc, &assist_lane, &assist_travel);\n'
    text=text[:end-1]+binding+text[end-1:]
    text='#include "karoq_v06_adas_ui_bridge.h"\n'+text
    ui_source.write_text(text,encoding='utf-8')
    return True,'已將原 V0.6 image 與四種具名資產直接綁定'

def run(source,dest,addon):
    source,dest,addon=map(lambda p:p.expanduser().resolve(),[source,dest,addon])
    check(source.is_dir(),'找不到 V1.0.5 來源專案')
    check(not dest.exists(),'目標目錄已存在，不會覆蓋')
    check(source!=dest and source not in dest.parents and dest not in source.parents,'來源與目標不可互相包含')
    base=source/'main'
    for rel in ('core/main.c','id/can/karoq_can_sniffer.c',
                'id/signals/karoq_mqb_opendbc.c','id/signals/karoq_mqb_opendbc.h',
                'other/diagnostics/karoq_mqb_openpilot_probe.c',
                'ui/v0_6/karoq_hud.c','ui/v0_6/karoq_hud_assets.c','CMakeLists.txt',
                'other/storage/karoq_sd_logger.c'):
        check((base/rel).is_file(),'V1.0.5 專案缺少 '+rel)
    check('karoq_mqb_assist_runtime' not in (base/'core/main.c').read_text(errors='replace'),
          '來源似乎已包含 V1.0.6；拒絕重複安裝')
    ui=base/'ui/v0_6'
    hashes={p.relative_to(ui).as_posix():sha(p) for p in ui.rglob('*') if p.is_file()}
    core=(base/'core/main.c').read_text(encoding='utf-8-sig')
    sniff=(base/'id/can/karoq_can_sniffer.c').read_text(encoding='utf-8-sig')
    cmake=(base/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    root=(source/'CMakeLists.txt').read_text(encoding='utf-8-sig')
    check(core.count('karoq_hud_ui_init();')==1,'原 V0.6 UI 初始化調用次數異常')
    check(core.count('karoq_mqb_openpilot_probe_init();')==1,'原 V1.0.5 探針初始化未找到')
    check(sniff.count('karoq_mqb_openpilot_probe_ingest(&msg);')==1,'原 CAN RX 探針 hook 未找到')
    check('"id/signals/karoq_mqb_opendbc.c"' in cmake,'CMake 缺少原 opendbc 解碼器')
    check(re.search(r'\bproject\s*\(\s*karoq_can_hud_v1_0_5\b',root),'來源不是 V1.0.5 專案')
    check('ui/v0_6' in cmake,'必須使用分支定稿 V0.6 UI')
    temp=dest.parent/(dest.name+'__V106_TEMP')
    check(not temp.exists(),'有未清理的升級暫存目錄')
    dest.parent.mkdir(parents=True,exist_ok=True)
    try:
        shutil.copytree(source,temp,ignore=lambda _,names:[n for n in names if n in ('build','.git','__pycache__','venv','.venv') or n.endswith('.pyc')])
        tmain=temp/'main'
        hist=temp/'history/V1_0_5_ADAS_UI_Link'
        for rel in ('core/main.c','id/can/karoq_can_sniffer.c','CMakeLists.txt','ui/v0_6/karoq_hud.c'):
            dst=hist/rel;dst.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(tmain/rel,dst)
        for rel in PATCH_FILES:
            dst=tmain/rel;dst.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(addon/'main'/rel,dst)
        sniff=sniff.replace('#include "karoq_mqb_openpilot_probe.h"',
            '#include "karoq_mqb_openpilot_probe.h"\n#include "karoq_mqb_assist_runtime.h"',1)
        sniff=sniff.replace('karoq_mqb_openpilot_probe_ingest(&msg);',
            'karoq_mqb_openpilot_probe_ingest(&msg);\n            karoq_mqb_assist_runtime_ingest(&msg);',1)
        check(sniff.count('karoq_mqb_assist_runtime_ingest(&msg);')==1,'CAN ingest hook 注入失敗')
        (tmain/'id/can/karoq_can_sniffer.c').write_text(sniff,encoding='utf-8')
        core='#include "karoq_mqb_assist_runtime.h"\n#include "karoq_v06_adas_ui_bridge.h"\n'+core
        # Important: runtime initialized BEFORE CAN starts, and UI timer in original LVGL lock.
        orig=re.search(r'(?m)^([ \t]*)([^\n]*\bkaroq_mqb_openpilot_probe_init\s*\(\s*\)[^\n]*;)',core)
        check(orig is not None,'找不到 MQB probe 初始化行')
        indent=orig.group(1)
        init=indent+'ESP_ERROR_CHECK(karoq_mqb_assist_runtime_init());\n'
        core=core[:orig.start()]+init+core[orig.start():]
        core=core.replace('karoq_hud_ui_init();',
             'karoq_hud_ui_init();\n        karoq_v06_adas_ui_attach();',1)
        check(core.count('karoq_v06_adas_ui_attach();')==1,'UI hook 未成功')
        (tmain/'core/main.c').write_text(core,encoding='utf-8')
        cmake=cmake.replace('"id/signals/karoq_mqb_opendbc.c"',
            '"id/signals/karoq_mqb_opendbc.c"\n        "id/signals/karoq_mqb_assist_state.c"\n        "other/diagnostics/karoq_mqb_assist_runtime.c"\n        "ui/bridge/karoq_v06_adas_ui_bridge.c"',1)
        (tmain/'CMakeLists.txt').write_text(cmake,encoding='utf-8')
        # UI bind only if asset and exact LVGL image source found. No unknown UI mutation.
        did_bind, message=patch_icon_binding(tmain/'ui/v0_6/karoq_hud.c')
        root=re.sub(r'\bproject\s*\(\s*karoq_can_hud_v1_0_5\b','project(karoq_can_hud_v1_0_6',root,count=1)
        (temp/'CMakeLists.txt').write_text(root,encoding='utf-8')
        (temp/'VERSION').write_text('1.0.6\nUI=CAN_SNIFFER_V0.6\nCAN=LISTEN_ONLY\nACC=TSK_06_CROSS_CHECKED\nLANE=OBSERVE_ONLY\nTRAVEL=OBSERVE_ONLY\n',encoding='utf-8')
        for folder in ('tools','tests','docs'):
            shutil.copytree(addon/folder,temp/folder,dirs_exist_ok=True)
        for name in ('README_安裝.md','CHANGELOG.md'):
            shutil.copy2(addon/name,temp/name)
        # Compare old bytes except possible verified icon bind in V0.6 UI.
        modified=[rel for rel,h in hashes.items() if sha(tmain/'ui/v0_6'/rel)!=h]
        check(modified==(['karoq_hud.c'] if did_bind else []), '非預期 V0.6 原始資料變動: '+repr(modified))
        for rel in ('id/decoders/karoq_can_speed.c','id/decoders/karoq_can_acc_probe.c',
                    'other/storage/karoq_sd_logger.c','other/buttons/karoq_record_button.c'):
            check(sha(base/rel)==sha(tmain/rel),'已驗證原模組被改變：'+rel)
        (temp/'docs/V106_UI_SHA256.json').write_text(json.dumps({
            'original':hashes,
            'final':{p.relative_to(tmain/'ui/v0_6').as_posix():sha(p) for p in (tmain/'ui/v0_6').rglob('*') if p.is_file()},
            'auto_bound':did_bind,'reason':message
        },ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        (temp/'docs/V106_LINK_STATUS.txt').write_text(
            'UI original image bind='+('BOUND' if did_bind else 'UNBOUND')+'\n'+message+'\n'+
            'If UNBOUND, CAN state is linked and logged but original V0.6 icon images will not change; do not pretend success.\n',encoding='utf-8')
        check(len(re.findall(r'\bkaroq_mqb_assist_runtime_init\s*\(\s*\)', core))==1,'init duplicated')
        temp.rename(dest)
        print('SUCCESS V1.0.6:',dest)
        print('MQB -> ADAS state -> LVGL timer link installed; V0.6 UI:',message)
        if not did_bind:
            print('WARNING: V0.6 icon remains unbound; open docs/V106_LINK_STATUS.txt. NO icon visual changes until precise asset binding.')
    except Exception:
        if temp.exists():shutil.rmtree(temp)
        raise

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--dest',type=Path,required=True);p.add_argument('--addon',type=Path,default=PKG)
    a=p.parse_args()
    try:run(a.source,a.dest,a.addon)
    except Exception as e:print('ERROR:',e,file=sys.stderr);sys.exit(1)
