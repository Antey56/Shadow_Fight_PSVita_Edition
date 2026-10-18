#!/usr/bin/env python3
"""Replay verified registration metadata into host C table; no ARM execution."""
import argparse
import hashlib
import json
import pathlib
from elf_dynamic import DynamicELF
from registration_host import Declaration,CodeRange,Method,load,functions

def verify(unity,profile,host_library):
    data=unity.read_bytes()
    if hashlib.sha256(data).hexdigest()!=profile['unity_sha256']:raise ValueError('Unity SHA mismatch')
    elf=DynamicELF(data);groups=profile['confirmed_registration_groups']
    declarations=[Declaration(g['class'].encode(),m['method'].encode(),m['descriptor'].encode())
                  for g in groups for m in g['methods']]
    policy=(Declaration*len(declarations))(*declarations)
    ranges=[CodeRange(p[2],p[2]+p[4]) for p in elf.loads if p[6]&1]
    code=(CodeRange*len(ranges))(*ranges)
    lib=load(host_library);env=lib.sf_jni_bridge_create(b'.',4096)
    if not env:raise RuntimeError('env allocation failed')
    verified=[]
    try:
        if lib.sf_jni_bridge_configure_natives(env,policy,len(policy),code,len(code)):raise AssertionError('configuration failed')
        fn=functions(env)
        for group in groups:
            cls=fn['find'](env,group['class'].encode())
            if not cls:raise AssertionError('class not found')
            methods=(Method*len(group['methods']))(*(Method(m['method'].encode(),m['descriptor'].encode(),m['function_pointer']) for m in group['methods']))
            if fn['register'](env,cls,methods,len(methods)):raise AssertionError('registration rejected')
            for m in group['methods']:
                address=lib.sf_jni_bridge_registered(env,group['class'].encode(),m['method'].encode(),m['descriptor'].encode())
                if address!=m['function_pointer']:raise AssertionError('lookup mismatch')
            verified.append({'class':group['class'],'methods_verified':len(methods)})
            fn['delete'](env,cls)
    finally:lib.sf_jni_bridge_destroy(env)
    return {'groups':verified,'methods_verified':sum(g['methods_verified'] for g in verified),
            'arm_code_executed':False,'addresses':'link-time ARM32 VA stored as integers only',
            'registration_metadata_replayed_on_host':True,'unity_sha256':profile['unity_sha256'],
            'execution_allowed':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('unity',type=pathlib.Path)
    p.add_argument('--profile',type=pathlib.Path,required=True);p.add_argument('--host-library',type=pathlib.Path,required=True)
    p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    r=verify(a.unity,json.loads(a.profile.read_text()),a.host_library)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(r,indent=2),encoding='utf-8')
    print('Registration metadata verified through JNI table:',r['methods_verified'],'methods; ARM executed: false')
