from pathlib import Path
import argparse,struct,hashlib,subprocess,re,json,time

def elf_info(path):
    blob=path.read_bytes()
    assert blob[:7]==b'\x7fELF\x01\x01\x01'
    shoff=struct.unpack_from('<I',blob,32)[0]
    shsize,shnum,shstr=struct.unpack_from('<HHH',blob,46)
    headers=[struct.unpack_from('<10I',blob,shoff+i*shsize) for i in range(shnum)]
    h=headers[shstr];names=blob[h[4]:h[4]+h[5]]
    sections={names[h[0]:].split(b'\0')[0].decode():h for h in headers}
    symbols={}
    for h in headers:
        if h[1]!=2:continue
        strings=headers[h[6]];strings=blob[strings[4]:strings[4]+strings[5]]
        for i in range(h[4],h[4]+h[5],h[9]):
            n,addr,_,_,_,index=struct.unpack_from('<IIIBBH',blob,i)
            name=strings[n:].split(b'\0')[0].decode()
            if name and index: symbols[name]=addr
    # Reject extension opcodes and CSR instructions, not merely undefined symbols.
    t=sections['.text'];code=blob[t[4]:t[4]+t[5]]
    for (w,) in struct.iter_unpack('<I',code):
        opcode=w&127;f3=(w>>12)&7;f7=w>>25
        assert w&3==3 and opcode in (0x03,0x13,0x17,0x23,0x33,0x37,0x63,0x67,0x6f,0x73),hex(w)
        if opcode==0x33:assert f7 in (0,32),hex(w)
        if opcode==0x73:assert w==0x73,hex(w)
        if opcode==0x03:assert f3 in (0,1,2,4,5)
        if opcode==0x23:assert f3 in (0,1,2)
    assert not any(n in symbols for n in ('__mulsi3','__divsi3','__udivsi3','__modsi3','__umodsi3'))
    def file_offset(addr):
        for h in headers:
            if h[1]!=8 and h[3]<=addr<h[3]+h[5]:return h[4]+addr-h[3]
        raise ValueError('Address has no file data')
    offset=file_offset(symbols['input_state'])
    data={n:sections[n][5] if n in sections else 0 for n in ('.text','.data','.bss','.rodata')}
    return blob,offset,data,symbols,file_offset

def replay(state,moves):
    p=[int(c)-1 for c in state[:7]];o=[int(c)-1 for c in state[7:]]
    source=[[1,4,2,0,3,5,6],[0,1,2,4,5,6,3],[0,2,5,3,1,4,6]]
    twist=[[1,2,0,2,1,0,0],[0,0,0,1,2,1,2],[0]*7]
    names=['R','R2',"R'",'B','B2',"B'",'D','D2',"D'"]
    for move in moves:
        face,turn=divmod(names.index(move),3)
        for _ in range(turn+1):
            p=[p[j] for j in source[face]]
            o=[(o[j]+twist[face][i])%3 for i,j in enumerate(source[face])]
    return p==list(range(7)) and o==[0]*7

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--ripes',required=True)
    parser.add_argument('--assembly',default='handwritten.elf')
    args=parser.parse_args()
    root=Path(__file__).resolve().parent
    inputs=[('12345671111111',0),('25314672313211',1),('63157423133333',6),('21345671111111',11),('54721631111111',11)]
    templates={'handwritten':elf_info(Path(args.assembly).resolve()),'gcc':elf_info(root/'gcc_reference.elf')}
    for name,size in [('p_distance',5040),('o_distance',729),('p_transition',90720),('o_transition',13122)]:
        a,b=templates.values()
        ao=a[4](a[3][name]);bo=b[4](b[3][name])
        assert a[0][ao:ao+size]==b[0][bo:bo+size],name+' mismatch'
    results=[]
    for kind,(blob,offset,sizes,_,_) in templates.items():
        for state,length in inputs:
            patch=bytearray(blob);patch[offset:offset+15]=state.encode()+b'\0'
            elf=root/(kind+'_'+state+'.elf');elf.write_bytes(patch)
            report=elf.with_suffix('.txt');report.unlink(missing_ok=True)
            run=subprocess.run([args.ripes,'--mode','cli','--src',str(elf),'-t','elf','--proc','RV32_ISS','--reginit','gpr:2=0x7ffffff0','--iret','--exectime','--timeout','60000','--output',str(report)],capture_output=True,timeout=75,creationflags=subprocess.CREATE_NO_WINDOW)
            console=run.stdout.decode(errors='replace').replace('\0','')
            elf.with_suffix('.console.txt').write_text(console,encoding='utf-8')
            text=report.read_text() if report.exists() else ''
            sol=re.search(r'solution \((\d+) moves\):([^\r\n]*)',console)
            count=re.search(r'instructions retired\s+(\d+)',text)
            correct=bool(run.returncode==0 and sol and count and int(sol[1])==length and len(sol[2].split())==length and replay(state,sol[2].split()))
            row=dict(implementation=kind,input=state,correct=correct,iret=int(count[1]) if count else None,text_bytes=sizes['.text'],solution=sol[2].strip() if sol else console)
            results.append(row);print(json.dumps(row),flush=True)
            assert correct,'Test failed'
    summary=dict(model='RV32_ISS',ripes_build='v2.2.6-106-g5b8a616',compiler='riscv32-unknown-elf-gcc 11.1.0',flags='-O2 -march=rv32i -mabi=ilp32',tables_identical=True,base_rv32i_audit_pass=True,includes_parsing_search_replay_output=True,sections={k:v[2] for k,v in templates.items()},elf_sha256={k:hashlib.sha256(v[0]).hexdigest() for k,v in templates.items()},results=results)
    (root/'comparison.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
    print('PASS: comparison.json saved',flush=True)
if __name__=='__main__':main()
