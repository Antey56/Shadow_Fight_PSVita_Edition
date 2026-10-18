#!/usr/bin/env python3
import argparse,pathlib,struct,hashlib
from elf_dynamic import DynamicELF
p=argparse.ArgumentParser();p.add_argument('elf',type=pathlib.Path);p.add_argument('header',type=pathlib.Path);a=p.parse_args()
b=a.elf.read_bytes();e=DynamicELF(b)
h=e.single(4);off=e.file_offset(h,8);_,count=struct.unpack_from('<II',b,off)
if count>1024:raise ValueError('unexpected fixture symbol count')
entry=[e.symbol(i) for i in range(count) if e.symbol(i)['name']=='sf_fixture_entry']
if len(entry)!=1 or not entry[0]['section_index'] or entry[0]['type']!=2:raise ValueError('fixture entry missing')
value=entry[0]['value']
if not any(x[6]&1 and x[2]<=value&~1 and (value&~1)+2<=x[2]+x[4] for x in e.loads):raise ValueError('entry outside code')
a.header.write_text('/* Generated from authored arm_smoke_fixture.c; SHA256 '+hashlib.sha256(b).hexdigest()+' */\n'
                    '#define SF_ARM_SMOKE_ENTRY '+hex(value)+'u\nstatic const unsigned char sf_arm_smoke_image[]={\n'+
                    ','.join(str(v) for v in b)+'\n};\n')
print('Embedded authored ARM fixture',len(b),'bytes, Thumb entry',hex(value))
