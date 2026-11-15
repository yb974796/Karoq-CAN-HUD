import tempfile,subprocess,sys,csv
from pathlib import Path
with tempfile.TemporaryDirectory() as t:
    base=Path(t)/'old.csv';cur=Path(t)/'new.csv';out=Path(t)/'compare.csv'
    hdr='uptime_ms,frame_type,id,dlc,rtr,data,event\n'
    base.write_text(hdr+'1,STD11,0x0FD,8,0,00,NEW_ID\n2,STD11,0x30C,8,0,00,NEW_ID\n')
    cur.write_text(hdr+'2,STD11,0x0FD,8,0,00,NEW_ID\n3,EXT29,0x1B00004A,2,0,12,NEW_ID\n')
    script=Path(__file__).resolve().parents[1]/'tools/compare_can_logs.py'
    subprocess.run([sys.executable,str(script),'--baseline',str(base),'--current',str(cur),'--output',str(out)],check=True)
    rows=list(csv.DictReader(out.open()))
    assert len(rows)==3
    assert sum(x['classification']=='NEW_VS_BASELINE' for x in rows)==1
    assert sum(x['classification']=='MISSING_VS_BASELINE' for x in rows)==1
    assert sum(x['classification']=='EXISTING' for x in rows)==1
    print('PASS: cross-session new, existing, and missing CAN ID classes')
