#!/usr/bin/env python3
"""Recover relocation-backed ARM32 JNINativeMethod candidates, never execute."""
import argparse
import collections
import hashlib
import json
import pathlib
import struct
from elf_dynamic import DynamicELF


def cstring(elf, address):
    # Require the whole string to remain in one file-backed PT_LOAD.
    for p in elf.loads:
        if p[2] <= address < p[2]+p[4]:
            off=p[1]+address-p[2]
            end=elf.b.find(b'\0',off,min(off+1024,p[1]+p[4]))
            if end<0:raise ValueError('unterminated string')
            return elf.b[off:end].decode('ascii')
    raise ValueError('unmapped string')


def scan(data, declarations):
    e=DynamicELF(data)
    by_pair=collections.defaultdict(list)
    for m in declarations:
        by_pair[m['method'],m['descriptor']].append(m['class'])
    rel={r['target_vaddr']:r for r in e.relocations() if r['type']}
    result=[]
    for address,r in sorted(rel.items()):
        # Three local pointer relocations are stronger evidence than a name.
        if any(rel.get(address+i,{}).get('type')!=23 or
               rel[address+i]['symbol_index']!=0 for i in (0,4,8)):continue
        try:
            name,sig,fn=struct.unpack_from('<III',data,e.file_offset(address,12))
            n=cstring(e,name);d=cstring(e,sig)
            classes=sorted(set(by_pair.get((n,d),[])))
            if not classes:continue
            code=fn&~1
            executable=any(p[6]&1 and p[2]<=code and code+2<=p[2]+p[4] for p in e.loads)
            if not executable:continue
            if not fn&1 and fn%4:continue
            result.append({'table_vaddr':address,'method':n,'descriptor':d,
                           'function_pointer':fn,'code_vaddr':code,
                           'instruction_set':'Thumb' if fn&1 else 'ARM',
                           'class_candidates':classes,'class_binding_confirmed':False,
                           'pointer_relocations':'three R_ARM_RELATIVE entries'})
        except (ValueError,UnicodeError):continue
    return result


def run(root,audit):
    modules=[]
    for path in sorted(root.glob('*.so')):
        data=path.read_bytes();candidates=scan(data,audit['native_methods'])
        modules.append({'module':path.name,'sha256':hashlib.sha256(data).hexdigest(),
                        'candidates':candidates})
    return {'apk_sha256':audit['sha256'],'modules':modules,
            'candidate_count':sum(len(m['candidates']) for m in modules),
            'execution_allowed':False,
            'limitations':['Name/signature and three local relocations identify table candidates, not executed RegisterNatives calls.',
                           'Class candidates come from matching DEX declarations; registration receiver and count require code tracing.',
                           'Only local R_ARM_RELATIVE triplets are handled; symbol-based or constructed tables may be missed.',
                           'Addresses are link-time virtual addresses, never ready-to-call Vita pointers.']}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('module_directory',type=pathlib.Path)
    p.add_argument('--audit',type=pathlib.Path,required=True)
    p.add_argument('--out',type=pathlib.Path,required=True)
    a=p.parse_args();r=run(a.module_directory,json.loads(a.audit.read_text(encoding='utf-8')))
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(r,indent=2),encoding='utf-8')
    print('Relocation-backed native table candidates:',r['candidate_count'])
