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
    (0x26ca80,25,'com/unity3d/player/UnityPlayer'),
    (0x26cac4,1,'com/google/androidgamesdk/ChoreographerCallback'),
    (0x26cb08,2,'com/google/androidgamesdk/SwappyDisplayManager'),
    (0x26b040,3,'com/unity3d/player/GoogleARCoreApi'),
    (0x24b7bc,4,'com/unity3d/player/Camera2Wrapper'),
    (0x267aa0,2,'com/unity3d/player/HFPStatus'),
    (0x269c24,1,'com/unity3d/player/AudioVolumeHandler'),
    (0x25a0f8,1,'com/unity3d/player/UnityCoreAssetPacksStatusCallbacks'),
    (0x252120,1,'com/unity3d/player/OrientationLockListener')]
ONLOAD_CALLS=[(0x26ca1e,0x19fd98),(0x26ca3c,0x26ca80),(0x26ca42,0x26cac4),
              (0x26ca48,0x26cb08),(0x26ca4e,0x26b040),(0x26ca54,0x24b7bc),
              (0x26ca5a,0x267aa0),(0x26ca60,0x269c24),(0x26ca66,0x25a0f8),(0x26ca6c,0x252120)]

def thumb_bl_target(first,second,address):
    if first&0xf800!=0xf000 or second&0xd000!=0xd000:raise ValueError('not Thumb BL')
    sign=(first>>10)&1;i1=1^((second>>13)&1)^sign;i2=1^((second>>11)&1)^sign
    displacement=(sign<<24)|(i1<<23)|(i2<<22)|((first&1023)<<12)|((second&2047)<<1)
    if sign:displacement-=1<<25
    return (address+4+displacement)&0xffffffff

def registration_layout(e,helper):
    start=helper
    if e.b[e.file_offset(helper,4):e.file_offset(helper,4)+4]==bytes.fromhex('00f000b8'):helper+=4
    def half(va):return struct.unpack_from('<H',e.b,e.file_offset(va,2))[0]
    if (half(helper)!=0xb5b0 or half(helper+8)!=0x447d or half(helper+0x22)!=0x447a or
        half(helper+2)&0xff00!=0x4d00 or half(helper+0x20)&0xff00!=0x4a00 or
        half(helper+0x18)&0xff00!=0x2300 or half(helper+0x24)!=0x47e0 or
        e.b[e.file_offset(helper+0x1a,4):e.file_offset(helper+0x1a,4)+4]!=bytes.fromhex('d0f85cc3')):
        raise ValueError('registration instruction profile mismatch')
    return dict(entry=start,body=helper,clit=((helper+6)&~3)+(half(helper+2)&255)*4,
                cadd=helper+8,tlit=((helper+0x24)&~3)+(half(helper+0x20)&255)*4,
                tadd=helper+0x22,count=half(helper+0x18)&255)



def relative(elf,literal,add_instruction):
    value=struct.unpack_from('<i',elf.b,elf.file_offset(literal,4))[0]
    return (value+add_instruction+4)&0xffffffff


def audit(data,declarations):
    if hashlib.sha256(data).hexdigest()!=UNITY_SHA:
        raise ValueError('Unity bootstrap profile SHA-256 mismatch; no address reuse permitted')
    e=DynamicELF(data);candidates={m['table_vaddr']:m for m in scan(data,declarations)}
    direct=[]
    for caller,expected_target in ONLOAD_CALLS:
        first,second=struct.unpack_from('<HH',data,e.file_offset(caller,4))
        target=thumb_bl_target(first,second,caller)
        if target!=expected_target:raise ValueError('JNI_OnLoad direct-call mismatch')
        direct.append(dict(caller_vaddr=caller,target_vaddr=target))
    groups=[]
    for helper,count,expected in REGISTRATIONS:
        layout=registration_layout(e,helper)
        if layout['count']!=count:raise ValueError('registration count mismatch')
        clit,cadd,tlit,tadd=(layout[k] for k in ('clit','cadd','tlit','tadd'))
        cell=relative(e,clit,cadd)
        name_ptr=struct.unpack_from('<I',data,e.file_offset(cell,4))[0]
        cls=cstring(e,name_ptr)
        if cls!=expected:raise ValueError('profile class mismatch')
        table=relative(e,tlit,tadd);methods=[]
        for i in range(count):
            m=dict(candidates[table+12*i])
            if 'L'+cls+';' not in m['class_candidates']:raise ValueError('DEX class mismatch')
            m['class_binding_confirmed']=True;m['class']=cls;methods.append(m)
        groups.append({'helper_vaddr':helper,'class':cls,'count':count,'body_vaddr':layout['body'],
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
            'jni_onload_direct_calls':direct,'pre_registration_timing_helper':0x19fd98,
            'confirmed_registration_groups':groups,'confirmed_method_count':sum(g['count'] for g in groups),
            'init_helper_vaddr':0x25cb90,'reflection_evidence':reflection,
            'observed_jni_slots':[6,18,21,31,33,113,215,219],
            'slot_observations':[
                {'function':'JNI_OnLoad registration helpers','slot':215,'load_instruction_vaddr':0x26ca9a,'table_offset_bytes':860},
                {'function':'initJni','slot':219,'load_instruction_vaddr':0x26c27e,'table_offset_bytes':876},
                {'function':'init helper','slot':113,'load_instruction_vaddr':0x25cc1c,'table_offset_bytes':452}],
            'limitations':['Observed slots are a lower bound, not a complete call graph.',
                           'FatalError slot 18 is observed on failure branches, not necessarily normal startup.',
                           'The profile confirms static class/table/count wiring for all nine reviewed direct registration helpers; Android code was never called.',
                           'The timing helper and transitive callees/constructors remain unproven; this is not a full startup call graph.',
                           'ELF virtual function pointers require validated relocation, executable mapping and runtime before use.'],
            'execution_allowed':False}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('unity',type=pathlib.Path)
    p.add_argument('--audit',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
    a=p.parse_args();r=audit(a.unity.read_bytes(),json.loads(a.audit.read_text(encoding='utf-8'))['native_methods'])
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(r,indent=2),encoding='utf-8')
    print('Confirmed static registration wiring:',r['confirmed_method_count'],'methods in',len(r['confirmed_registration_groups']),'groups')
