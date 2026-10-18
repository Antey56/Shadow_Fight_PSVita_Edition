#!/usr/bin/env python3
"""Plan ARM32 ELF lifecycle calls as data only. Never executes module code."""
import argparse, hashlib, json, pathlib, struct
from elf_dynamic import DynamicELF


def plan(data, base=0x90000000):
    elf=DynamicELF(data)
    minimum=min(p[2] for p in elf.loads)
    end=max(p[2]+p[5] for p in elf.loads)
    if base<0 or base%4096 or base+end-minimum>0xffffffff:
        raise ValueError('invalid runtime reservation')
    bias=(base-minimum)&0xffffffff
    rels={r['target_vaddr']:r for r in elf.relocations() if r['type']}
    def target(value):
        raw=value&~1
        width=2 if value&1 else 4
        if not value or (not value&1 and raw%4):raise ValueError('invalid function alignment')
        if not any(p[6]&1 and p[2]<=raw and raw+width<=p[2]+p[4] for p in elf.loads):
            raise ValueError('lifecycle target is not file-backed executable code')
        return {'vaddr':hex(value),'runtime':hex((bias+value)&0xffffffff),'thumb':bool(value&1)}
    def one(tag):
        value=elf.single(tag,0)
        return [] if not value else [target(value)]
    def array(pointer_tag,size_tag):
        present=(pointer_tag in elf.tags,size_tag in elf.tags)
        if present[0]!=present[1]:raise ValueError('incomplete lifecycle array tags')
        if not present[0]:return []
        address=elf.single(pointer_tag);size=elf.single(size_tag)
        if address%4 or size%4 or size//4>4096:raise ValueError('invalid lifecycle array size/alignment')
        if not size:return []
        off=elf.file_offset(address,size)
        result=[]
        for i in range(size//4):
            slot=address+i*4;value=struct.unpack_from('<I',data,off+i*4)[0]
            relocation=rels.get(slot)
            # Only local R_ARM_RELATIVE entries are resolvable without a namespace.
            if relocation and (relocation['type']!=23 or relocation['symbol_index']):
                raise ValueError('lifecycle entry needs namespace symbol resolution')
            if not relocation:
                if value in (0,0xffffffff):continue
                raise ValueError('unrelocated lifecycle function pointer')
            call=target(value);call['array_index']=i;call['slot_vaddr']=hex(slot);result.append(call)
        return result
    preinit=array(32,33)
    if preinit:raise ValueError('PREINIT_ARRAY unsupported for shared objects')
    init=one(12)+array(25,27)
    fini=list(reversed(array(26,28)))+one(13)
    return {'sha256':hashlib.sha256(data).hexdigest(),'base':hex(base),'load_span':end-minimum,
            'init_count':len(init),'fini_count':len(fini),'init_order':init,'fini_order':fini,
            'execution_allowed':False,'dependencies_initialized':False,
            'limitations':['Local relative lifecycle entries only; no namespace binding.',
                           'Planning does not resolve imports, certify ABI, or execute constructors.',
                           'Dependencies must initialize before this module; TLS and thread lifecycle remain unsupported.']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module',type=pathlib.Path);parser.add_argument('--out',type=pathlib.Path,required=True)
    args=parser.parse_args();result=plan(args.module.read_bytes())
    args.out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('init',result['init_count'],'fini',result['fini_count'],'execution allowed: false')
