#!/usr/bin/env python3
"""Revalidate a manually reviewed libmain bootstrap profile; never executes ARM."""
import argparse,hashlib,json,pathlib,struct
from elf_dynamic import DynamicELF
from audit_unity_bootstrap import relative
from scan_native_tables import cstring,scan
MAIN_SHA='dad352d6cc2a8cf184d4e0a9dfe6d7f3768c329b4c684587c28e48eb01511d71'
CLASS='com/unity3d/player/NativeLoader'
def audit(data,declarations):
    if hashlib.sha256(data).hexdigest()!=MAIN_SHA:raise ValueError('libmain SHA-256 mismatch')
    e=DynamicELF(data)
    cls=cstring(e,relative(e,0x1138,0x10fe));table=relative(e,0x113c,0x1110)
    if cls!=CLASS or table!=0x5004:raise ValueError('registration profile mismatch')
    # Count and JNI function loads reviewed in Thumb disassembly.
    for va,expected in [(0x10f0,b'\x0b\x69'),(0x10fc,b'\x92\x69'),
                        (0x1106,b'\x02\x23'),(0x110a,b'\xd2\xf8\x5c\xc3'),
                        (0x12e0,b'\x01\x21'),(0x12ec,b'\x01\x21')]:
        if data[e.file_offset(va,len(expected)):e.file_offset(va,len(expected))+len(expected)]!=expected:
            raise ValueError('instruction profile mismatch')
    candidates={m['table_vaddr']:m for m in scan(data,declarations)};methods=[]
    for i,expected in enumerate([('load','(Ljava/lang/String;)Z',0x1145),('unload','()Z',0x11e1)]):
        m=dict(candidates[table+12*i])
        if (m['method'],m['descriptor'],m['function_pointer'])!=expected or 'L'+CLASS+';' not in m['class_candidates']:
            raise ValueError('method profile mismatch')
        m.update(class_binding_confirmed=True);m['class']=CLASS;methods.append(m)
    dynamic=[]
    for literal,add,expected in [(0x11d0,0x11a2,'libunity.so'),(0x11d8,0x11b4,'libil2cpp.so'),(0x137c,0x12fa,'JNI_OnLoad')]:
        address=relative(e,literal,add)
        if cstring(e,address)!=expected:raise ValueError('dynamic loader string mismatch')
        dynamic.append(dict(string=expected,vaddr=address,literal_vaddr=literal,add_vaddr=add))
    return dict(module='libmain.so',sha256=MAIN_SHA,jni_onload_pointer=0x10e5,
                confirmed_registration_groups=[dict(helper_vaddr=0x10e4,table_vaddr=table,count=2,**{'class':CLASS},methods=methods)],
                confirmed_method_count=2,observed_jni_slots=[6,18,168,169,170,215,219],
                observed_vm_slots=[4],dynamic_loader_evidence=dynamic,
                load_order=['libunity.so','libil2cpp.so'],
                dlopen_flags=1,dlopen_flag_profile='Both reviewed calls set ARM32 RTLD_LAZY=1; path attempt then basename fallback.',
                profile_evidence='Exact-hash profile from manual Thumb review; PC-relative strings, count, key slot instructions and relocated native table revalidated.',
                execution_allowed=False,game_executed=False,
                limitations=['Static metadata and instructions only; load order is not successful execution.',
                             'ELF constructors, dlopen/dlsym, dependency resolution, unwind/TLS/ABI and Java lifecycle remain required.',
                             'Observed JNI/VM slots are a lower bound, not a complete engine call graph.'])
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('libmain',type=pathlib.Path)
    p.add_argument('--audit',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
    a=p.parse_args();r=audit(a.libmain.read_bytes(),json.loads(a.audit.read_text(encoding='utf-8'))['native_methods'])
    a.out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8');print('NativeLoader registrations confirmed:',r['confirmed_method_count'])
