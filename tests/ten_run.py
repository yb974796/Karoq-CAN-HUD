#!/usr/bin/env python3
"""10 independent cycles of host-based tests. Not equivalent to ESP-IDF board testing."""
import json,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
result=[]
for round_no in range(1,11):
    start=time.monotonic()
    p=subprocess.run(['bash',str(root/'tests/run_host_tests.sh')],
      capture_output=True,text=True,timeout=40)
    result.append({'run':round_no,'status':'PASS' if p.returncode==0 else 'FAIL',
      'duration_ms':round((time.monotonic()-start)*1000),
      'exit_code':p.returncode,'output':p.stdout[-2000:],'errors':p.stderr[-2000:]})
    print(f'10run {round_no:02d}/10: {result[-1]["status"]}')
    if p.returncode!=0:
        print(p.stdout,p.stderr,file=sys.stderr)
        break
output=root/'tests/10run_results.json'
output.write_text(json.dumps({'host_only':True,'cycles':result,'total_passed':sum(x['status']=='PASS' for x in result)},indent=2)+'\n')
print('Report:',output)
if len(result)!=10 or result[-1]['status']!='PASS':sys.exit(1)
