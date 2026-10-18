#!/usr/bin/env python3
"""Validate a manually disassembled bootstrap profile for one exact Unity ELF."""
import argparse
import hashlib
import json
import pathlib
import struct
from elf_dynamic import DynamicELF
from scan_native_tables import cstring,scan

UNITY_SHA='82e24ba84e18a85beb7b81f221d4405b8f3befd94fc7025d8b8282c8d1d9535b'
# Reviewed Thumb code in JNI_OnLoad, not a general-purpose disassembler.
REGISTRATIONS=[
    (0x26ca80,0x26cabc,0x26ca88,0x26cac0,0x26caa2,25,'com/unity3d/player/UnityPlayer'),
    (0x26cac4,0x26cb00,0x26cacc,0x26cb04,0x26cae6,1,'com/google/androidgamesdk/ChoreographerCallback'),
    (0x26cb08,0x26cb44,0x26cb10,0x26cb48,0x26cb2a,2,'com/google/androidgamesdk/SwappyDisplayManager')]


def relative(elf,literal,add_instruction):
    value=struct.unpack_from('<i',elf.b,elf.file_offset(literal,4))[0]
    return (value+add_instruction+4)&0xffffffff


def audit(data,declarations):
    if hashlib.sha256(data).hexdigest()!=UNITY_SHA:
        raise ValueError('Unity bootstrap profile SHA-256 mismatch; no address reuse permitted')
    e=DynamicELF(data);candidates={m['table_vaddr']:m for m in scan(data,declarations)}
    groups=[]
    for helper,clit,cadd,tlit,tadd,count,expected in REGISTRATIONS:
        cell=relative(e,clit,cadd)
        name_ptr=struct.unpack_from('<I',data,e.file_offset(cell,4))[0]
        cls=cstring(e,name_ptr)
        if cls!=expected:raise ValueError('profile class mismatch')
        table=relative(e,tlit,tadd);methods=[]
        for i in range(count):
            m=dict(candidates[table+12*i])
            if 'L'+cls+';' not in m['class_candidates']:raise ValueError('DEX class mismatch')
            m['class_binding_confirmed']=True;m['class']=cls;methods.append(m)
        groups.append({'helper_vaddr':helper,'class':cls,'count':count,
                       'table_vaddr':table,'methods':methods})
    reflection=[]
    for literal,add,expected in [
        (0x25cc58,0x25cbe0,'java/lang/ClassLoader'),
        (0x25cc5c,0x25cbf0,'getClassLoader'),
        (0x25cc60,0x25cbf2,'()Ljava/lang/ClassLoader;'),
        (0x25cc68,0x25cc20,'forName'),
        (0x25cc6c,0x25cc22,'(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;')]:
        address=relative(e,literal,add)
        if cstring(e,address)!=expected:raise ValueError('reflection profile mismatch')
        reflection.append({'string':expected,'vaddr':address,'literal_vaddr':literal})
    return {'unity_sha256':UNITY_SHA,'profile_evidence':'Manual Thumb disassembly reviewed; exact ELF hash and PC-relative data revalidated by script.',
            'jni_onload_pointer':0x26ca19,'init_jni_pointer':0x26c26d,
            'confirmed_registration_groups':groups,'confirmed_method_count':sum(g['count'] for g in groups),
            'init_helper_vaddr':0x25cb90,'reflection_evidence':reflection,
            'observed_jni_slots':[6,18,21,31,33,113,215,219],
            'slot_observations':[
                {'function':'JNI_OnLoad registration helpers','slot':215,'load_instruction_vaddr':0x26ca9a,'table_offset_bytes':860},
                {'function':'initJni','slot':219,'load_instruction_vaddr':0x26c27e,'table_offset_bytes':876},
                {'function':'init helper','slot':113,'load_instruction_vaddr':0x25cc1c,'table_offset_bytes':452}],
            'limitations':['Observed slots are a lower bound, not a complete call graph.',
                           'FatalError slot 18 is observed on failure branches, not necessarily normal startup.',
                           'The profile confirms static class/table/count wiring for three groups only; Android code was never called.',
                           'ELF virtual function pointers require validated relocation, executable mapping and runtime before use.'],
            'execution_allowed':False}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('unity',type=pathlib.Path)
    p.add_argument('--audit',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
    a=p.parse_args();r=audit(a.unity.read_bytes(),json.loads(a.audit.read_text(encoding='utf-8'))['native_methods'])
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(r,indent=2),encoding='utf-8')
    print('Confirmed static registration wiring:',r['confirmed_method_count'],'methods in',len(r['confirmed_registration_groups']),'groups')
