#!/usr/bin/env python3
"""Offline-only evidence report: original SD CSV OR original KAROQ_CAN RX NEW_ID text logs.
Does not guess that a missing ID means an ECU is absent.
"""
import argparse,csv,re,sys,collections
from pathlib import Path

MQB={
 0x0fd:('ESP_21','speed observed'),0x0b2:('ESP_19','wheels observed'),
 0x30b:('Kombi_01','cluster speed observed'),0x120:('TSK_06','ACC core'),
 0x30c:('ACC_02','ACC display observed'),0x122:('ACC_06','ACC status'),
 0x12b:('GRA_ACC_01','ACC buttons observed'),0x397:('LDW_02','camera lane warning'),
 0x09f:('LH_EPS_03','EPS observed'),0x126:('HCA_01','camera steering'),
 0x30f:('SWA_01','blind spot observed'),0x366:('Blinkmodi_02','turn signals observed'),
}
RX=re.compile(r'RX\s+(STD11|EXT29)\s+id=(0x[0-9a-fA-F]+)\s+dlc=(\d+)\s+rtr=(\d+).*?data=\[([^]]*)\]')

def rows(path):
    t=path.read_text(encoding='utf-8-sig',errors='replace')
    if t.lstrip().startswith('uptime_ms,frame_type,id,'):
        for r in csv.DictReader(t.splitlines()):
            try:
                yield int(r['id'],0),r['frame_type'],int(r['dlc']),int(r['rtr']),r['data']
            except (KeyError,ValueError):continue
    else:
        for s in t.splitlines():
            m=RX.search(s)
            if m:yield int(m[2],16),m[1],int(m[3]),int(m[4]),m[5]

def review(paths):
    observations=collections.Counter();lengths=collections.Counter();all_ids=set()
    for path in paths:
        for id_,frame_type,dlc,rtr,data in rows(path):
            all_ids.add((frame_type,id_))
            if id_ in MQB and frame_type=='STD11' and rtr==0 and dlc==8:
                observations[id_]+=1
                lengths[id_]+=len(data.split())
    print('Input:',', '.join(p.name for p in paths))
    print('Distinct CAN IDs:',len(all_ids),'MQB known 11-bit ID seen:',len(observations),'/',len(MQB))
    for id_,(label,comment) in MQB.items():
        status=f'SEEN ({observations[id_]} frames)' if observations[id_] else 'NOT OBSERVED IN INPUT'
        print(f'  0x{id_:03X} {label:15} {status:26} {comment}')
    print('Note: NEW_ID-only text log is a sparse snapshot, not continuous CAN traffic.')
    print('Note: absent IDs may be on different CAN buses, filtered by gateway, or inactive.')
    return observations
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('files',nargs='+',type=Path);a=ap.parse_args()
    try:review(a.files)
    except OSError as e:print(e,file=sys.stderr);sys.exit(1)
