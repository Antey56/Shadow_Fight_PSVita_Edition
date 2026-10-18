#!/usr/bin/env python3
"""Embed only the exact locally supplied libmain, never download game code."""
import argparse,pathlib,hashlib
from audit_libmain_bootstrap import MAIN_SHA
p=argparse.ArgumentParser();p.add_argument('elf',type=pathlib.Path);p.add_argument('header',type=pathlib.Path);a=p.parse_args()
b=a.elf.read_bytes()
if hashlib.sha256(b).hexdigest()!=MAIN_SHA:raise ValueError('libmain probe SHA-256 mismatch')
a.header.write_text('/* Local game probe, SHA256 '+MAIN_SHA+'; do not publish this generated header. */\n'
                    'static const unsigned char sf_libmain_probe_image[]={\n'+','.join(str(x) for x in b)+'\n};\n')
print('Embedded local hash-verified libmain',len(b),'bytes; generated header stays local')
