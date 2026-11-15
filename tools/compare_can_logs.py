#!/usr/bin/env python3
"""Compare all CAN IDs, including disappeared IDs, from two RAW CSV recordings."""
import argparse,csv,collections
from pathlib import Path

def ids(path):
    counts=collections.Counter()
    first={}
    with Path(path).open(newline='',encoding='utf-8-sig') as f:
        for row in csv.DictReader(f):
            key=(row['frame_type'],row['id'].upper())
            counts[key]+=1
            first.setdefault(key,row['uptime_ms'])
    return counts,first

def main():
    p=argparse.ArgumentParser(description='Karoq V1.0.4 CAN ID comparison')
    p.add_argument('--current',required=True)
    p.add_argument('--baseline',required=True)
    p.add_argument('--output',default='CAN_compare.csv')
    a=p.parse_args()
    old,_=ids(a.baseline);new,first=ids(a.current)
    with open(a.output,'w',newline='',encoding='utf-8') as f:
        w=csv.writer(f)
        w.writerow(['frame_type','can_id','current_count','baseline_count','first_uptime_ms','classification'])
        for key in sorted(set(old)|set(new)):
            status=('NEW_VS_BASELINE' if key not in old else
                    'MISSING_VS_BASELINE' if key not in new else 'EXISTING')
            w.writerow([key[0],key[1],new[key],old[key],first.get(key,''),status])
    print('baseline IDs:',len(old),'current IDs:',len(new),
          'new:',len(set(new)-set(old)),'missing:',len(set(old)-set(new)))
    print('report:',a.output)
if __name__=='__main__':main()
