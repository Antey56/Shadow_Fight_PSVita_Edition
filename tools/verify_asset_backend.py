#!/usr/bin/env python3
"""Verify authored C file backend against selected real APK assets; no execution."""
import argparse,ctypes as C,hashlib,json,os,pathlib,tempfile,zipfile
from asset_host import load_backend
def verify(apk,dll):
    lib=load_backend(dll);reports=[]
    local=pathlib.Path(__file__).resolve().parents[1]/'local-input';local.mkdir(exist_ok=True)
    candidates=['assets/bin/Data/boot.config','assets/bin/Data/globalgamemanagers','assets/bin/Data/globalgamemanagers.assets','assets/bin/Data/Managed/Metadata/global-metadata.dat']
    with tempfile.TemporaryDirectory(dir=local) as tmp,zipfile.ZipFile(apk) as z:
        root=pathlib.Path(tmp).resolve();names=[n for n in candidates if n in z.namelist()]
        if len(names)<2:raise ValueError('expected Unity asset samples not found')
        expected={}
        for name in names:
            info=z.getinfo(name)
            if info.file_size>32*1024**2:raise ValueError('sample exceeds 32 MiB verification cap')
            dest=root/name.removeprefix('assets/');dest.parent.mkdir(parents=True,exist_ok=True);digest=hashlib.sha256()
            with z.open(name) as src,dest.open('wb') as out:
                for data in iter(lambda:src.read(65536),b''):digest.update(data);out.write(data)
            expected[name]=digest.hexdigest()
        manager=lib.sf_asset_manager_create(os.fsencode(root),16*1024**2)
        if not manager:raise ValueError('C manager creation failed')
        try:
            for name in names:
                a=lib.AAssetManager_open(manager,name.removeprefix('assets/').encode(),2)
                if not a:raise ValueError('C open failed for sample')
                try:
                    length=lib.AAsset_getLength64(a)
                    if length!=z.getinfo(name).file_size:raise AssertionError('length mismatch')
                    buf=C.create_string_buffer(65536);digest=hashlib.sha256()
                    while True:
                        n=lib.AAsset_read(a,buf,len(buf))
                        if n<0:raise AssertionError('C read failed')
                        if not n:break
                        digest.update(buf.raw[:n])
                    if digest.hexdigest()!=expected[name]:raise AssertionError('stream SHA mismatch')
                    if lib.AAsset_getRemainingLength64(a)!=0:raise AssertionError('remaining mismatch at EOF')
                    probes=[0,length//2,max(0,length-37)]
                    for pos in probes:
                        if lib.AAsset_seek64(a,pos,0)!=pos:raise AssertionError('seek mismatch')
                        n=lib.AAsset_read(a,buf,73)
                        with z.open(name) as src:src.seek(pos);ref=src.read(73)
                        if n!=len(ref) or buf.raw[:n]!=ref:raise AssertionError('seek/read data mismatch')
                    lib.AAsset_seek64(a,0,0);p=lib.AAsset_getBuffer(a)
                    if length<=16*1024**2:
                        if not p or hashlib.sha256(C.string_at(p,length)).hexdigest()!=expected[name]:raise AssertionError('buffer mismatch')
                        if lib.AAsset_getRemainingLength64(a)!=length:raise AssertionError('getBuffer changed stream position')
                    elif p:raise AssertionError('buffer limit bypassed')
                    reports.append({'asset':name,'bytes':length,'sha256':expected[name],'stream_matches_apk':True,'seek_probes':len(probes),'buffer_matches_apk':bool(p),'game_executed':False})
                finally:lib.AAsset_close(a)
        finally:lib.sf_asset_manager_destroy(manager)
    return {'samples':reports,'sample_bytes':sum(x['bytes'] for x in reports),'note':'Only hashes/results published. Extracted samples were temporary and removed. No Android code executed.'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('apk',type=pathlib.Path);p.add_argument('--host-library',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    report=verify(a.apk,a.host_library);a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(report,indent=2),encoding='utf-8');print('Verified',len(report['samples']),'APK assets;',report['sample_bytes'],'bytes; temporary samples removed.')
