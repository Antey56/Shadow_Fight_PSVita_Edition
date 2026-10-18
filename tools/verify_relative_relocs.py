#!/usr/bin/env python3
"""Compare host C RELATIVE relocation arithmetic against Python on real ELF files.
Only authored host DLL is loaded. Android code is NEVER executed."""
import argparse,ctypes as C,json,pathlib,struct,hashlib
from elf_dynamic import DynamicELF
class Rel(C.Structure):
    _fields_=[('offset',C.c_uint32),('type',C.c_uint32),('symbol_value',C.c_uint32),('symbol_resolved',C.c_int),('thumb',C.c_int)]
def verify(path,fn):
    b=path.read_bytes()
    if b[:7]!=b'\x7fELF\x01\x01\x01' or struct.unpack_from('<H',b,18)[0]!=40:raise ValueError('requires ARM32 LE ELF')
    h=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
    ph=[struct.unpack_from('<IIIIIIII',b,h[5]+i*h[9]) for i in range(h[10])]
    loads=[p for p in ph if p[0]==1 and p[5]];low=min(p[2] for p in loads);high=max(p[2]+p[5] for p in loads)
    if high-low>256*1024**2:raise ValueError('test image exceeds 256 MiB')
    original=bytearray(high-low)
    for p in loads:original[p[2]-low:p[2]-low+p[4]]=b[p[1]:p[1]+p[4]]
    rels=[];nonzero=0
    for i in range(h[12]):
        s=struct.unpack_from('<IIIIIIIIII',b,h[6]+i*h[11])
        if s[1]!=9:continue
        if s[9]!=8:raise ValueError('unexpected Elf32_Rel stride')
        for off in range(s[4],s[4]+s[5],8):
            place,info=struct.unpack_from('<II',b,off)
            if info&255!=23:continue
            if info>>8:nonzero+=1;continue
            if not any(p[2]<=place and place+4<=p[2]+p[5] for p in loads):raise ValueError('relocation outside LOAD')
            rels.append(Rel(place-low,23,0,0,0))
    dynamic=DynamicELF(b).relocations()
    dynamic_rels=[Rel(r['target_vaddr']-low,23,0,0,0) for r in dynamic if r['type']==23 and r['symbol_index']==0]
    if sorted(r.offset for r in dynamic_rels)!=sorted(r.offset for r in rels):raise AssertionError('PT_DYNAMIC disagrees with section parser')
    bias=0x10000000 # synthetic arithmetic constant, NOT Vita address reservation
    image=(C.c_ubyte*len(original)).from_buffer_copy(original);arr=(Rel*len(dynamic_rels))(*dynamic_rels);failed=C.c_size_t()
    rc=fn(image,len(original),bias,arr,len(rels),C.byref(failed))
    if rc:raise ValueError('C relocation failed: '+str(rc))
    expected=bytearray(original)
    for r in rels:
        addend=struct.unpack_from('<I',original,r.offset)[0]
        struct.pack_into('<I',expected,r.offset,(bias+addend)&0xffffffff)
    actual=bytes(image)
    if actual!=expected:raise AssertionError('C result differs from Python oracle')
    return {'module':path.name,'relative_verified':len(rels),'relative_nonzero_symbol_not_tested':nonzero,'image_bytes':len(original),'result_sha256':hashlib.sha256(actual).hexdigest(),'game_code_executed':False,'c_input_source':'PT_DYNAMIC','python_oracle_source':'section_headers'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--host-library',type=pathlib.Path,required=True);p.add_argument('--modules',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    dll=C.CDLL(str(a.host_library.resolve()));fn=dll.arm_reloc_apply
    fn.argtypes=[C.POINTER(C.c_ubyte),C.c_size_t,C.c_uint32,C.POINTER(Rel),C.c_size_t,C.POINTER(C.c_size_t)];fn.restype=C.c_int
    results=[verify(path,fn) for path in sorted(a.modules.glob('*.so'))]
    if not results:raise ValueError('no modules found')
    a.out.write_text(json.dumps({'modules':results,'relative_verified':sum(r['relative_verified'] for r in results),'limitations':['Only RELATIVE symbol-index-zero relocations tested on APK.','Other types are covered by synthetic C tests, not full ELF linkage.','No instructions, constructors, JNI or game lifecycle executed.']},indent=2),encoding='utf-8')
    print('Verified real RELATIVE entries:',sum(r['relative_verified'] for r in results),'across',len(results),'modules')
