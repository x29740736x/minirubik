"""Small, deliberately restricted RV32I assembler for this reference program.
Emits ELF32 with explicit .text/.data addresses, avoiding Ripes CLI section-base
settings. It does NOT compile C or optimize/generate the algorithm.
Only the mnemonics used in solver_core.s are supported. .align is in BYTES,
matching this Ripes version; use .balign when assembling with GNU tools.
Usage: python assemble.py solver_rv32i.s [output.elf]
"""
import ast,json,re,struct,sys
from pathlib import Path

REGNAMES='zero ra sp gp tp t0 t1 t2 s0 s1 a0 a1 a2 a3 a4 a5 a6 a7 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 t3 t4 t5 t6'.split()
REG={r:i for i,r in enumerate(REGNAMES)}|{f'x{i}':i for i in range(32)}|{'fp':8}
BASE={'.text':0,'.data':0x10000000,'.rodata':0x10010000}

def assemble(src,out):
    sec='.text';sizes={k:0 for k in BASE};labels={};records=[]
    for line_no,line in enumerate(Path(src).read_text(encoding='utf-8-sig').splitlines(),1):
        line=line.split('#',1)[0].strip()
        if not line:continue
        if re.match(r'^[A-Za-z_][A-Za-z0-9_]*:', line):
            label,line=line.split(':',1);labels[label.strip()]=(sec,sizes[sec]);line=line.strip()
            if not line:continue
        parts=line.split(None,1);op=parts[0];arg=parts[1] if len(parts)>1 else ''
        if op in BASE:sec=op;continue
        if op=='.section':
            assert arg in BASE
            sec=arg;continue
        if op in ('.globl','.global'):continue
        data=None
        if op=='.align':
            n=int(arg,0);data=bytes((-sizes[sec])%n) if n else b''
        elif op=='.zero':data=bytes(int(arg,0))
        elif op=='.asciz':data=ast.literal_eval(arg).encode()+b'\0'
        elif op in ('.byte','.half','.word'):
            n={'.byte':1,'.half':2,'.word':4}[op]
            data=b''.join((int(v.strip(),0)&((1<<(n*8))-1)).to_bytes(n,'little') for v in arg.split(','))
        elif op.startswith('.'):raise ValueError((line_no,'unsupported directive',line))
        if data is not None:n=len(data)
        elif op=='la':n=8
        elif op=='li':n=4 if -2048<=int(arg.split(',')[1],0)<=2047 else 8
        else:n=4
        records.append((sec,sizes[sec],line_no,line,data,n));sizes[sec]+=n
    sym={k:BASE[s]+o for k,(s,o) in labels.items()}
    sections={s:bytearray() for s in BASE};listing=[]
    def I(opc,rd,f3,rs1,imm):
        assert -2048<=imm<=2047
        return ((imm&4095)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|opc
    def R(rd,f3,rs1,rs2,f7=0):return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|0x33
    for s,offset,line_no,line,data,n in records:
        if data is None:
            p=re.split(r'[\s,()]+',line);op=p[0];a=p[1:];pc=BASE[s]+offset
            def r(i):return REG[a[i]]
            words=[]
            if op in ('li','la'):
                val=int(a[1],0) if op=='li' else sym[a[1]]
                if op=='li' and n==4:words=[I(0x13,r(0),0,0,val)]
                else:
                    hi=(val+0x800)>>12;lo=val-(hi<<12)
                    words=[(hi<<12)|(r(0)<<7)|0x37,I(0x13,r(0),0,r(0),lo)]
            elif op=='mv':words=[I(0x13,r(0),0,r(1),0)]
            elif op=='ret':words=[I(0x67,0,0,1,0)]
            elif op=='ecall':words=[0x73]
            elif op in ('add','sub','or','and','sltu','sll'):
                words=[R(r(0),{'add':0,'sub':0,'or':6,'and':7,'sltu':3,'sll':1}[op],r(1),r(2),32 if op=='sub' else 0)]
            elif op in ('addi','slli','srli'):
                imm=int(a[2],0)
                if op!='addi':assert 0<=imm<32
                words=[I(0x13,r(0),{'addi':0,'slli':1,'srli':5}[op],r(1),imm)]
            elif op in ('lw','lbu','lhu'):
                words=[I(3,r(0),{'lw':2,'lbu':4,'lhu':5}[op],r(2),int(a[1],0))]
            elif op in ('sw','sb','sh'):
                imm=int(a[1],0);assert -2048<=imm<=2047;imm&=4095
                words=[((imm>>5)<<25)|(r(0)<<20)|(r(2)<<15)|(({'sw':2,'sh':1,'sb':0}[op])<<12)|((imm&31)<<7)|0x23]
            elif op in ('beq','bne','blt','bge','bltu','bgeu'):
                imm=sym[a[2]]-pc;assert -4096<=imm<=4094 and imm%2==0;imm&=8191
                words=[((imm>>12)<<31)|(((imm>>5)&63)<<25)|(r(1)<<20)|(r(0)<<15)|({'beq':0,'bne':1,'blt':4,'bge':5,'bltu':6,'bgeu':7}[op]<<12)|(((imm>>1)&15)<<8)|(((imm>>11)&1)<<7)|0x63]
            elif op in ('j','jal'):
                rd=0 if op=='j' else r(0);target=a[0] if op=='j' else a[1]
                imm=sym[target]-pc;assert -(1<<20)<=imm<(1<<20) and imm%2==0;imm&=(1<<21)-1
                words=[((imm>>20)<<31)|(((imm>>1)&1023)<<21)|(((imm>>11)&1)<<20)|(((imm>>12)&255)<<12)|(rd<<7)|0x6f]
            else:raise ValueError((line_no,'unsupported instruction',line))
            data=b''.join(struct.pack('<I',w) for w in words)
            assert len(data)==n
            listing.append(f'{pc:08x} {data.hex():16s} {line}')
        assert len(sections[s])==offset
        sections[s].extend(data)
    # ELF32 executable, separate executable, writable, and read-only segments.
    textoff=0x100;dataoff=(textoff+sizes['.text']+15)&~15
    blob=bytearray(dataoff+sizes['.data'])
    blob[textoff:textoff+sizes['.text']]=sections['.text'];blob[dataoff:]=sections['.data']
    blob.extend(bytes((-len(blob))%16));rooff=len(blob);blob.extend(sections['.rodata'])
    shstr=b'\0.text\0.data\0.shstrtab\0.strtab\0.symtab\0.rodata\0'
    shstroff=len(blob);blob.extend(shstr)
    strs=bytearray(b'\0');syms=bytearray(16)
    for name,(s,offset) in labels.items():
        no=len(strs);strs.extend(name.encode()+b'\0')
        syms.extend(struct.pack('<IIIBBH',no,BASE[s]+offset,0,0,0,{'.text':1,'.data':2,'.rodata':6}[s]))
    stroff=len(blob);blob.extend(strs)
    blob.extend(bytes((-len(blob))%4));symoff=len(blob);blob.extend(syms)
    shoff=len(blob)
    sh=[(0,0,0,0,0,0,0,0,0,0),(1,1,6,0,textoff,sizes['.text'],0,0,4,0),(7,1,3,BASE['.data'],dataoff,sizes['.data'],0,0,16,0),(13,3,0,0,shstroff,len(shstr),0,0,1,0),(23,3,0,0,stroff,len(strs),0,0,1,0),(31,2,0,0,symoff,len(syms),4,len(syms)//16,4,16),(39,1,2,BASE['.rodata'],rooff,sizes['.rodata'],0,0,16,0)]
    for row in sh:blob.extend(struct.pack('<10I',*row))
    ident=b'\x7fELF'+bytes([1,1,1,0])+bytes(8)
    blob[:52]=struct.pack('<16sHHIIIIIHHHHHH',ident,2,243,1,sym['main'],52,shoff,0,52,32,3,40,7,3)
    blob[52:84]=struct.pack('<8I',1,textoff,0,0,sizes['.text'],sizes['.text'],5,4)
    blob[84:116]=struct.pack('<8I',1,dataoff,BASE['.data'],BASE['.data'],sizes['.data'],sizes['.data'],6,16)
    blob[116:148]=struct.pack('<8I',1,rooff,BASE['.rodata'],BASE['.rodata'],sizes['.rodata'],sizes['.rodata'],4,16)
    out=Path(out);out.write_bytes(blob)
    out.with_suffix('.listing.txt').write_text('\n'.join(listing)+'\n')
    info={'text_bytes':sizes['.text'],'data_bytes':sizes['.data'],'bss_bytes':0,'rodata_bytes':sizes['.rodata'],'static_total':sizes['.data']+sizes['.rodata'],'ISA':'RV32I','entry':sym['main'],'symbols':sym}
    out.with_suffix('.layout.json').write_text(json.dumps(info,indent=2))
    print(f'{out.name}: .text={sizes[".text"]}, static data={info["static_total"]} bytes')
    return info

if __name__=='__main__':
    src=Path(sys.argv[1]);assemble(src,sys.argv[2] if len(sys.argv)>2 else src.with_suffix('.elf'))
