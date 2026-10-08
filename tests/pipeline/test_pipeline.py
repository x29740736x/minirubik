from pathlib import Path
import argparse,subprocess,json,re,hashlib,time
from compare import elf_info,replay

p=argparse.ArgumentParser()
p.add_argument('--elf',required=True)
p.add_argument('--ripes',required=True)
p.add_argument('--out',default='pipeline_results')
args=p.parse_args()
elf_path=Path(args.elf).resolve();directory=Path(args.out).resolve();directory.mkdir(exist_ok=True,parents=True)
blob,offset,sections,_,_=elf_info(elf_path)
fingerprint=hashlib.sha256(blob).hexdigest()
rows=[];begin=time.perf_counter()
for state,length in [('12345671111111',0),('25314672313211',1),('21345671111111',11)]:
    print('Running RV32_5S:',state,'expected moves:',length,flush=True)
    patch=bytearray(blob);patch[offset:offset+15]=state.encode()+b'\0'
    src=directory/(state+'.elf');src.write_bytes(patch)
    report=directory/(state+'.txt');report.unlink(missing_ok=True)
    run=subprocess.run([args.ripes,'--mode','cli','--src',str(src),'-t','elf','--proc','RV32_5S','--reginit','gpr:2=0x7ffffff0','--iret','--exectime','--timeout','600000','--output',str(report)],capture_output=True,timeout=660,creationflags=subprocess.CREATE_NO_WINDOW)
    console=run.stdout.decode(errors='replace').replace('\0','');text=report.read_text() if report.exists() else ''
    (directory/(state+'.console.txt')).write_text(console,encoding='utf-8')
    sol=re.search(r'solution \((\d+) moves\):([^\r\n]*)',console)
    im=re.search(r'instructions retired\s+(\d+)',text)
    tm=re.search(r'wall-clock model execution time \(ms\)\s+(\d+(?:\.\d+)?)',text)
    ok=bool(run.returncode==0 and sol and im and int(sol[1])==length and len(sol[2].split())==length and replay(state,sol[2].split()))
    row=dict(input=state,expected_moves=length,correct=ok,iret=int(im[1]) if im else None,model_time_ms=float(tm[1]) if tm else None,solution=sol[2].strip() if sol else console)
    rows.append(row)
    summary=dict(model='RV32_5S',ripes_build='v2.2.6-106-g5b8a616',elf_sha256=fingerprint,includes_output_and_replay=True,tested=len(rows),correct=sum(r['correct'] for r in rows),sections=sections,results=rows,wall_seconds=time.perf_counter()-begin)
    (directory/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
    print(json.dumps(row),flush=True)
    assert ok,'Pipeline test failed'
print('PASS: all three RV32_5S cases',flush=True)
