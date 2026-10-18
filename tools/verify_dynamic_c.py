#!/usr/bin/env python3
"""Compare authored C PT_DYNAMIC parser to Python on local ARM libraries."""
import argparse,ctypes as C,json,pathlib,struct
from elf_dynamic import DynamicELF
class RawRel(C.Structure):
    _fields_=[('target_vaddr',C.c_uint32),('type',C.c_uint32),('symbol_index',C.c_uint32)]
class Info(C.Structure):
    _fields_=[('load_min',C.c_uint32),('load_end',C.c_uint32),('strtab_file_offset',C.c_uint32),('strtab_size',C.c_uint32),('symtab_vaddr',C.c_uint32),('relocation_count',C.c_size_t)]
def load(path):
    dll=C.CDLL(str(path.resolve()));fn=dll.arm_dynamic_read
    fn.argtypes=[C.POINTER(C.c_ubyte),C.c_size_t,C.POINTER(RawRel),C.c_size_t,C.POINTER(Info)];fn.restype=C.c_int
    return fn
def read(fn,b):
    buf=(C.c_ubyte*len(b)).from_buffer_copy(b);info=Info()
    rc=fn(buf,len(b),None,0,C.byref(info))
    if rc:return rc,[],info
    arr=(RawRel*info.relocation_count)();rc=fn(buf,len(b),arr,len(arr),C.byref(info))
    return rc,[(r.target_vaddr,r.type,r.symbol_index) for r in arr] if not rc else [],info
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--host-library',type=pathlib.Path,required=True);p.add_argument('--modules',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    fn=load(a.host_library);reports=[]
    for path in sorted(a.modules.glob('*.so')):
        b=path.read_bytes();py=DynamicELF(b).relocations();rc,rels,info=read(fn,b)
        expected=[(r['target_vaddr'],r['type'],r['symbol_index']) for r in py]
        if rc or rels!=expected:raise AssertionError((path.name,rc,'C/Python mismatch'))
        stripped=bytearray(b);struct.pack_into('<I',stripped,32,0);struct.pack_into('<HHH',stripped,46,0,0,0)
        rc,other,_=read(fn,stripped)
        if rc or other!=expected:raise AssertionError('C requires section headers')
        reports.append({'module':path.name,'relocations_verified':len(rels),'matches_python':True,'works_without_sections':True,'game_executed':False})
    if not reports:raise ValueError('no modules')
    a.out.write_text(json.dumps(reports,indent=2),encoding='utf-8')
    print('C/Python exact relocation match:',sum(r['relocations_verified'] for r in reports),'records in',len(reports),'modules')
