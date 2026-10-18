#!/usr/bin/env python3
"""Additional non-executing Unity/DEX audit, standard library only."""
import argparse, collections, csv, hashlib, io, json, pathlib, re, struct, zipfile
from analyze_apk import analyze, dex_strings, u32

def uleb(b,p):
    v=0
    for shift in range(0,35,7):
        c=b[p];p+=1;v|=(c&127)<<shift
        if c<128:return v,p
    raise ValueError('oversized ULEB128')

def native_methods(b):
    strings=dex_strings(b)
    tn,to=u32(b,64),u32(b,68);types=[strings[u32(b,to+4*i)] for i in range(tn)]
    pn,po=u32(b,72),u32(b,76);protos=[]
    for i in range(pn):
        _,ret,params=struct.unpack_from('<III',b,po+12*i)
        args=[] if not params else [types[struct.unpack_from('<H',b,params+4+2*j)[0]] for j in range(u32(b,params))]
        protos.append('('+''.join(args)+')'+types[ret])
    mn,mo=u32(b,88),u32(b,92);methods=[]
    for i in range(mn):
        cls,proto,name=struct.unpack_from('<HHI',b,mo+8*i)
        methods.append((types[cls],strings[name],protos[proto]))
    cn,co=u32(b,96),u32(b,100);out=[]
    for i in range(cn):
        p=u32(b,co+32*i+24)
        if not p:continue
        counts=[]
        for _ in range(4):v,p=uleb(b,p);counts.append(v)
        for _ in range(counts[0]+counts[1]):
            _,p=uleb(b,p);_,p=uleb(b,p)
        for count in counts[2:]:
            idx=0
            for _ in range(count):
                diff,p=uleb(b,p);flags,p=uleb(b,p);code,p=uleb(b,p);idx+=diff
                if flags&0x100:
                    cls,name,sig=methods[idx];out.append({'class':cls,'method':name,'descriptor':sig,'static':bool(flags&8),'flags':hex(flags)})
    return out

def category(name):
    if name.startswith('egl'):return 'egl'
    if name.startswith(('ANativeWindow','ALooper','ASensor','AAsset','__android','__system_property')):return 'android'
    if name.startswith(('pthread','sem_')):return 'threads'
    if name.startswith(('__aeabi','_Unwind','__gnu','__cxa')):return 'compiler_runtime'
    if name in ('__sF','__errno','environ','__stack_chk_guard'):return 'bionic_data_or_tls'
    if name in ('fork','execl','popen','pclose','syscall','ptrace','mmap','mprotect','sigaction','sigaltstack'):return 'os_semantics'
    return 'review_signature_and_abi'

def run(apk,out):
    out.mkdir(parents=True,exist_ok=True);r=analyze(apk)
    result={'sha256':r['apk']['sha256'],'native_methods':[],'unity_versions':[],'metadata':None,'boot_config':None,'dynamic_api_strings':{},'notes':['DEX declarations give descriptors, not native registration addresses or lifecycle ordering.','Dynamic API strings are candidates; presence is not proof of invocation.']}
    with zipfile.ZipFile(apk) as z:
        for n in z.namelist():
            if re.fullmatch(r'classes\d*\.dex',n):
                result['native_methods'] += [dict(x,dex=n) for x in native_methods(z.read(n))]
            if n=='assets/bin/Data/boot.config':result['boot_config']=z.read(n).decode('utf-8','replace')
            if n.endswith('global-metadata.dat'):
                b=z.read(n);result['metadata']={'path':n,'size':len(b),'magic':hex(u32(b,0)),'version':u32(b,4),'sha256':hashlib.sha256(b).hexdigest()}
            if n.endswith(('libunity.so','globalgamemanagers')):
                b=z.read(n)
                result['unity_versions']+=sorted(set(x.decode() for x in re.findall(rb'20\d\d\.\d+\.\d+[abfp]\d+',b)))
                if n.endswith('libunity.so'):
                    ss={x.decode('ascii') for x in re.findall(rb'[\x20-\x7e]{4,}',b)}
                    result['dynamic_api_strings'][n]=sorted(x for x in ss if re.fullmatch(r'(gl|egl|vk|SL|AAudio|AMedia)[A-Za-z0-9_]+',x))
    result['unity_versions']=sorted(set(result['unity_versions']))
    (out/'deep-audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    with (out/'imports.csv').open('w',newline='',encoding='utf-8-sig') as f:
        w=csv.writer(f);w.writerow(['module','symbol','weak','category','implementation_status'])
        for lib in r['libraries']:
            for s in lib['imports']:w.writerow([lib['path'],s['name'],s['weak'],category(s['name']),'UNIMPLEMENTED'])
    lines=['# Native JNI declarations','', '| Class | Method | Descriptor | Static |','|---|---|---|---|']
    lines += ['| '+ ' | '.join([x['class'],x['method'],x['descriptor'],str(x['static'])])+' |' for x in result['native_methods']]
    (out/'JNI-METHODS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(json.dumps({'unity_versions':result['unity_versions'],'metadata':result['metadata'],'native_methods':len(result['native_methods']),'imports':sum(len(x['imports']) for x in r['libraries'])},ensure_ascii=False))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('apk',type=pathlib.Path);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args();run(a.apk,a.out)
