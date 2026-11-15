#!/usr/bin/env python3
"""Read-only report showing candidate 88/demo/speed writers in the private V0.6 UI source."""
import argparse,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--file',type=Path,required=True);a=p.parse_args()
lines=a.file.read_text(encoding='utf-8-sig',errors='replace').splitlines()
pat=re.compile(r'(?i)\b88\b|speed|geschw|kmh|lv_label_set_text|lv_timer_create|demo')
idx=[i for i,line in enumerate(lines) if pat.search(line)]
seen=set()
print('SOURCE:',a.file, 'matches:',len(idx))
for i in idx:
    if i in seen:continue
    lo,hi=max(0,i-1),min(len(lines),i+2)
    for j in range(lo,hi):
        if j not in seen:
            print(f'{j+1:5}: {lines[j][:220]}');seen.add(j)
    print('---')
print('NOTE: report only; no source modification. Remove demo timer only after exact owner is confirmed.')
