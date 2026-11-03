#!/usr/bin/env python3
"""DATA-ONLY C/Python lifecycle comparison. External imports get synthetic
integer placeholders ONLY inside a host byte buffer, never executable mappings.
This is not ABI/import certification or a runnable game library."""
import argparse,ctypes as C,hashlib,json,pathlib
from elf_lifecycle import plan as python_plan
class Linked(C.Structure):
    _fields_=[('bytes',C.POINTER(C.c_ubyte)),('size',C.c_size_t),('vaddr_min',C.c_uint32),('bias',C.c_uint32)]
class Error(C.Structure):
    _fields_=[('code',C.c_int),('relocation_index',C.c_size_t),('missing_symbol',C.c_char*128)]
class Export(C.Structure):
    _fields_=[('value',C.c_uint32),('size',C.c_uint32),('binding',C.c_uint),('type',C.c_uint),('absolute',C.c_uint)]
class Plan(C.Structure):
    _fields_=[('init',C.c_uint32*1024),('fini',C.c_uint32*1024),('init_count',C.c_size_t),
              ('fini_count',C.c_size_t),('base',C.c_uint32),('image_size',C.c_size_t),('validated',C.c_int)]
Resolver=C.CFUNCTYPE(C.c_int,C.c_char_p,C.c_uint,C.c_uint,C.POINTER(C.c_uint32),C.c_void_p)
def verify(path,build):
    def library(name):
        return C.CDLL(str(next(p for p in (build/(name+'.dll'),build/(name+'.so')) if p.exists()).resolve()))
    linker=library('arm_exec_host');exports=library('arm_symbols_host')
    linker.arm_link_image.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32,C.c_size_t,Resolver,C.c_void_p,C.POINTER(Linked),C.POINTER(Error)]
    linker.arm_link_dispose.argtypes=[C.POINTER(Linked)]
    linker.arm_lifecycle_plan.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(Linked),C.POINTER(Plan)]
    exports.arm_symbol_find.argtypes=[C.c_void_p,C.c_size_t,C.c_char_p,C.c_uint32,C.POINTER(Export)]
    data=path.read_bytes();expected=python_plan(data);buf=C.create_string_buffer(data);cache={};synthetic={};callback_error=[]
    base=0x90000000
    # The supplied game modules have minimum LOAD vaddr 0; derive it explicitly.
    from elf_dynamic import DynamicELF
    minimum=min(p[2] for p in DynamicELF(data).loads);bias=(base-minimum)&0xffffffff
    def resolve(name,binding,typ,value,ctx):
        try:
            key=(name,typ)
            if key not in cache:
                own=Export();rc=exports.arm_symbol_find(buf,len(data),name,bias,C.byref(own))
                if rc<0:raise ValueError('C export lookup failed: '+str(rc))
                if rc:
                    if typ and own.type and typ!=own.type:raise ValueError('own export type mismatch')
                    cache[key]=own.value
                else:
                    if name not in synthetic:synthetic[name]=0x81000000+16*len(synthetic)
                    cache[key]=synthetic[name]|(1 if typ in (0,2) else 0)
            value[0]=cache[key];return 1
        except Exception as exc:callback_error.append(str(exc));return -1
    callback=Resolver(resolve);image=Linked();error=Error();result=Plan()
    rc=linker.arm_link_image(buf,len(data),base,64*1024*1024,callback,None,C.byref(image),C.byref(error))
    if rc:raise ValueError('data-only C linking failed: '+str(rc)+' '+str(callback_error))
    try:
        rc=linker.arm_lifecycle_plan(buf,len(data),C.byref(image),C.byref(result))
        if rc:raise ValueError('C lifecycle planning failed: '+str(rc))
        for label in ('init','fini'):
            count=getattr(result,label+'_count');actual=list(getattr(result,label)[:count])
            wanted=[int(x['runtime'],16) for x in expected[label+'_order']]
            if actual!=wanted:raise ValueError(label+' C/Python mismatch')
        return dict(module=path.name,sha256=hashlib.sha256(data).hexdigest(),init_count=result.init_count,
                    fini_count=result.fini_count,c_python_order_match=True,synthetic_external_names=len(synthetic),
                    full_import_abi_certified=False,executable_mapping_created=False,constructors_executed=False,
                    execution_allowed=False,limitations=['Host data buffer uses synthetic external import integers; not callable implementations.',
                    'Comparison validates lifecycle metadata/order only. No Android ARM code executes.'])
    finally:linker.arm_link_dispose(C.byref(image))
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('module',type=pathlib.Path)
    parser.add_argument('--build',type=pathlib.Path,required=True);parser.add_argument('--out',type=pathlib.Path,required=True)
    args=parser.parse_args();r=verify(args.module,args.build)
    args.out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8');print(json.dumps(r))
