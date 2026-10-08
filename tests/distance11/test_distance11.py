from pathlib import Path
from concurrent.futures import ThreadPoolExecutor,as_completed
import struct,json,re,subprocess,hashlib,time,argparse
p=argparse.ArgumentParser()
p.add_argument("--elf",default="solver_generated.elf")
p.add_argument("--ripes",required=True)
p.add_argument("--workers",type=int,default=4)
p.add_argument("--limit",type=int,default=2644)
args=p.parse_args()
root=Path(__file__).resolve().parent
elf_path=Path(args.elf).resolve()
template=elf_path.read_bytes();fingerprint=hashlib.sha256(template).hexdigest()
assert template[:4]==b'\x7fELF'
layout=json.loads(elf_path.with_suffix('.layout.json').read_text());address=layout['symbols']['input_state']
phoff=struct.unpack_from('<I',template,28)[0];phsize,phnum=struct.unpack_from('<HH',template,42)
offset=None
for i in range(phnum):
    typ,off,addr,_,size,*_=struct.unpack_from('<8I',template,phoff+i*phsize)
    if typ==1 and addr<=address<addr+size:offset=off+address-addr
assert offset is not None
states=(root/'distance11.txt').read_text().splitlines();assert len(states)==len(set(states))==2644
assert 1<=args.limit<=2644 and 1<=args.workers<=16
states=states[:args.limit]
directory=elf_path.parent/('distance11_results' if args.limit==2644 else 'distance11_smoke');directory.mkdir(exist_ok=True)
ripes=str(Path(args.ripes).resolve())
assert Path(ripes).is_file()
def replay(s,moves):
    p=[int(c)-1 for c in s[:7]];o=[int(c)-1 for c in s[7:]]
    sources=[[1,4,2,0,3,5,6],[0,1,2,4,5,6,3],[0,2,5,3,1,4,6]];twists=[[1,2,0,2,1,0,0],[0,0,0,1,2,1,2],[0]*7]
    names=['R','R2',"R'",'B','B2',"B'",'D','D2',"D'"]
    for move in moves:
        face,turns=divmod(names.index(move),3)
        for _ in range(turns+1):
            p=[p[j] for j in sources[face]];o=[(o[j]+twists[face][i])%3 for i,j in enumerate(sources[face])]
    return p==list(range(7)) and o==[0]*7
def test(s):
    rowfile=directory/(s+'.json')
    if rowfile.exists():
        row=json.loads(rowfile.read_text())
        if row.get('elf_sha256')==fingerprint and row.get('correct') and row.get('iret') is not None:return row
    elf=bytearray(template);elf[offset:offset+15]=s.encode()+b'\0'
    src=directory/(s+'.elf');src.write_bytes(elf)
    report=directory/(s+'.txt');report.unlink(missing_ok=True)
    run=subprocess.run([ripes,'--mode','cli','--src',str(src),'-t','elf','--proc','RV32_ISS','--reginit','gpr:2=0x7ffffff0','--iret','--exectime','--timeout','60000','--output',str(report)],capture_output=True,timeout=75,creationflags=subprocess.CREATE_NO_WINDOW)
    console=run.stdout.decode(errors='replace').replace(chr(0),'');telemetry=report.read_text() if report.exists() else ''
    solution=re.search(r'solution \((\d+) moves\):([^\r\n]*)',console);im=re.search(r'instructions retired\s+(\d+)',telemetry)
    row=dict(input=s,elf_sha256=fingerprint,correct=bool(run.returncode==0 and im and solution and int(solution[1])==11 and len(solution[2].split())==11 and replay(s,solution[2].split())),iret=int(im[1]) if im else None,solution=solution[2].strip() if solution else None)
    rowfile.write_text(json.dumps(row,indent=2),encoding='utf-8')
    src.unlink(missing_ok=True)
    return row
begin=time.perf_counter();rows=[]
with ThreadPoolExecutor(max_workers=args.workers) as pool:
    futures=[pool.submit(test,s) for s in states]
    for future in as_completed(futures):
        row=future.result();rows.append(row)
        summary=dict(model='RV32_ISS',elf_sha256=fingerprint,includes_output_and_replay=True,tested=len(rows),correct=sum(r['correct'] for r in rows),missing_iret=sum(r['iret'] is None for r in rows),over_50000000=sum(r['iret'] is not None and r['iret']>50000000 for r in rows),complete_all_2644=len(rows)==2644,worst=max((r for r in rows if r['iret'] is not None),key=lambda r:r['iret'],default=None),wall_seconds=time.perf_counter()-begin)
        if len(rows)%20==0 or not row['correct'] or len(rows)==2644:
            (directory/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
        if len(rows)%100==0 or not row['correct']:print(json.dumps(summary),flush=True)
(directory/'results.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
print(json.dumps(summary),flush=True)
