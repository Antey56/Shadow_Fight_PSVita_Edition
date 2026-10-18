#!/usr/bin/env python3
"""Read-only APK inventory. Python 3.10+, standard library only; never executes APK code."""
import argparse, collections, hashlib, json, pathlib, re, struct, zipfile

def u16(b,o): return struct.unpack_from('<H',b,o)[0]
def u32(b,o): return struct.unpack_from('<I',b,o)[0]
def cstr(b,o):
    if o >= len(b): return ''
    return b[o:b.find(b'\0',o) if b.find(b'\0',o)>=0 else len(b)].decode('utf-8','replace')

def manifest(b):
    # Android ResXMLTree + ResStringPool; preserve all start-tag attributes.
    if b.lstrip().startswith(b'<'):
        import xml.etree.ElementTree as E
        root=E.fromstring(b)
        return [{'tag':e.tag,'attributes':dict(e.attrib)} for e in root.iter()]
    if u16(b,0)!=3: raise ValueError('not Android binary XML')
    strings=[]; tags=[]; p=u16(b,2)
    while p+8<=len(b):
        typ,hs,size=struct.unpack_from('<HHI',b,p)
        if size<hs or size<8 or p+size>len(b): raise ValueError('invalid XML chunk')
        if typ==1:
            n=u32(b,p+8); flags=u32(b,p+16); start=p+u32(b,p+20)
            def length(q,utf8):
                v=b[q] if utf8 else u16(b,q); q+=1 if utf8 else 2
                mask=128 if utf8 else 32768
                if v&mask:
                    v=((v&(mask-1))<<(8 if utf8 else 16))+(b[q] if utf8 else u16(b,q)); q+=1 if utf8 else 2
                return v,q
            for i in range(n):
                q=start+u32(b,p+hs+4*i)
                if flags&256:
                    _,q=length(q,True); ln,q=length(q,True); s=b[q:q+ln].decode('utf-8','replace')
                else:
                    ln,q=length(q,False); s=b[q:q+2*ln].decode('utf-16le','replace')
                strings.append(s)
        elif typ==0x102:
            def s(i): return strings[i] if i!=0xffffffff and i<len(strings) else None
            base=p+16; tag=s(u32(b,base+4)); astart=u16(b,base+8); astride=u16(b,base+10); count=u16(b,base+12); attrs={}
            for i in range(count):
                a=base+astart+i*astride; ns=s(u32(b,a)); name=s(u32(b,a+4)); raw=u32(b,a+8); t=b[a+15]; val=u32(b,a+16)
                value=s(raw) if raw!=0xffffffff else s(val) if t==3 else bool(val) if t==18 else val
                attrs[(ns+'}' if ns else '')+str(name)]=value
            tags.append({'tag':tag,'attributes':attrs})
        p+=size
    return tags

def elf(b):
    if b[:4]!=b'\x7fELF' or b[5]!=1: raise ValueError('not little-endian ELF')
    bits=b[4]
    if bits not in (1,2): raise ValueError('invalid ELF class')
    fmt='<16sHHIIIIIHHHHHH' if bits==1 else '<16sHHIQQQIHHHHHH'
    h=struct.unpack_from(fmt,b); shoff=h[6]; stride=h[11]; n=h[12]; namesidx=h[13]
    sf='<IIIIIIIIII' if bits==1 else '<IIQQQQIIQQ'; secs=[struct.unpack_from(sf,b,shoff+i*stride) for i in range(n)]
    def data(s): return b[s[4]:s[4]+s[5]]
    names=data(secs[namesidx]) if n else b''
    imports=[]; exports=[]; needed=[]; relocs=collections.Counter(); sections=[]; loads=[]
    pf='<IIIIIIII' if bits==1 else '<IIQQQQQQ'
    for i in range(h[10]):
        ph=struct.unpack_from(pf,b,h[5]+i*h[9])
        if ph[0]==1:
            loads.append({'vaddr':ph[2] if bits==1 else ph[3],'memsz':ph[5] if bits==1 else ph[6], 'offset':ph[1] if bits==1 else ph[2], 'filesz':ph[4] if bits==1 else ph[5], 'flags':ph[6] if bits==1 else ph[1], 'align':ph[7]})
    for sec in secs:
        name=cstr(names,sec[0]); sections.append({'name':name,'size':sec[5]})
        if sec[1]==11:
            st=data(secs[sec[6]])
            for off in range(sec[4],sec[4]+sec[5],sec[9] or (16 if bits==1 else 24)):
                if bits==1: ni,v,sz,info,other,idx=struct.unpack_from('<IIIBBH',b,off)
                else: ni,info,other,idx,v,sz=struct.unpack_from('<IBBHQQ',b,off)
                sym=cstr(st,ni)
                if sym:
                    if idx==0: imports.append({'name':sym,'weak':info>>4==2})
                    elif info>>4 in (1,2): exports.append(sym)
        if sec[1]==6:
            st=data(secs[sec[6]])
            for off in range(sec[4],sec[4]+sec[5],sec[9] or (8 if bits==1 else 16)):
                tag,val=struct.unpack_from('<II' if bits==1 else '<QQ',b,off)
                if tag==1: needed.append(cstr(st,val))
        if sec[1] in (4,9):
            for off in range(sec[4],sec[4]+sec[5],sec[9] or (8 if bits==1 else 16)):
                info=u32(b,off+4) if bits==1 else struct.unpack_from('<Q',b,off+8)[0]
                relocs[str(info & (255 if bits==1 else 0xffffffff))]+=1
    printable=[x.decode('ascii','replace') for x in re.findall(rb'[\x20-\x7e]{5,}',b)]
    pattern=re.compile(r'JNI|Java_|RegisterNatives|google|admob|license|billing|cocos|unity|android/|com/',re.I)
    return {'bits':32 if bits==1 else 64,'machine':h[2],'type':h[1],'flags':hex(h[7]),'needed':needed,'imports':imports,'exports':exports,'jni_exports':[x for x in exports if x.startswith(('Java_','JNI_'))],'relocations':dict(relocs),'sections':sections,'load_segments':loads,'load_span':max((x['vaddr']+x['memsz'] for x in loads),default=0)-min((x['vaddr'] for x in loads),default=0),'interesting_strings':sorted(set(x for x in printable if pattern.search(x)))[:2000],'limitations':['Section-header based symbols; stripped section tables require external readelf/dynamic-segment analysis.','String matches are leads, not proof of reachable calls.']}

def dex_strings(b):
    if not b.startswith(b'dex\n'): raise ValueError('not DEX')
    n,off=u32(b,56),u32(b,60); out=[]
    for i in range(n):
        p=u32(b,off+4*i)
        for _ in range(5):
            v=b[p]; p+=1
            if v<128: break
        out.append(cstr(b,p))
    return out

def analyze(path):
    digest=hashlib.sha256()
    with open(path,'rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): digest.update(chunk)
    r={'schema_version':1,'apk':{'name':path.name,'size':path.stat().st_size,'sha256':digest.hexdigest()},'manifest':[],'libraries':[],'assets':[],'dex':[],'errors':[]}
    with zipfile.ZipFile(path) as z:
        infos=z.infolist()
        if len({x.filename for x in infos})!=len(infos): raise ValueError('duplicate ZIP entries')
        for x in infos:
            n=x.filename
            if n.startswith('/') or '..' in pathlib.PurePosixPath(n).parts or '\\' in n: raise ValueError('unsafe ZIP path')
            if x.file_size>512*1024*1024: raise ValueError('entry exceeds 512 MiB safety cap')
            try:
                if n=='AndroidManifest.xml': r['manifest']=manifest(z.read(x))
                if n.startswith('lib/') and n.endswith('.so'):
                    e=elf(z.read(x)); e.update(path=n,size=x.file_size); r['libraries'].append(e)
                if n.startswith('assets/') and not x.is_dir():
                    ext=pathlib.PurePosixPath(n).suffix.lower()
                    kind='bundle' if ext=='.bundle' else 'texture' if ext in ('.png','.jpg','.jpeg','.pvr','.ktx','.dds','.webp','.pkm') else 'config_candidate' if ext in ('.xml','.json','.ini','.cfg','.lua') or re.search(r'/(users?[^/]*|userdata)(\.|/|$)',n,re.I) else 'other'
                    r['assets'].append({'path':n,'size':x.file_size,'kind':kind})
                if re.fullmatch(r'classes\d*\.dex',n):
                    ss=dex_strings(z.read(x)); matches=[s for s in ss if re.search(r'google|admob|license|billing|cocos|unity|native|loadLibrary',s,re.I)]
                    r['dex'].append({'path':n,'string_count':len(ss),'interesting_strings':matches[:5000],'note':'Strings only; no Java execution or control-flow proof; use JADX for methods.'})
            except Exception as ex: r['errors'].append({'path':n,'error':str(ex)})
    r['summary']={'abis':sorted({x['path'].split('/')[1] for x in r['libraries']}),'asset_count':len(r['assets']),'asset_bytes':sum(x['size'] for x in r['assets']),'asset_kinds':dict(collections.Counter(x['kind'] for x in r['assets'])),'armv7_present':any('/armeabi-v7a/' in x['path'] and x['machine']==40 and x['bits']==32 for x in r['libraries'])}
    return r

def main():
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('apk',type=pathlib.Path); p.add_argument('--out',type=pathlib.Path,default=pathlib.Path('report.json')); p.add_argument('--extract',type=pathlib.Path,help='Extract manifest, DEX, lib and assets into a NEW directory (max 2 GiB total)'); a=p.parse_args()
    try: r=analyze(a.apk)
    except Exception as ex: p.exit(2,'Analysis failed: '+str(ex)+'\n')
    if a.extract:
        try:
            with zipfile.ZipFile(a.apk) as z:
                entries=[x for x in z.infolist() if not x.is_dir() and (x.filename.startswith(('lib/','assets/')) or x.filename=='AndroidManifest.xml' or re.fullmatch(r'classes\d*\.dex',x.filename))]
                if sum(x.file_size for x in entries)>2*1024**3: raise ValueError('extraction exceeds 2 GiB')
                a.extract.mkdir(parents=True,exist_ok=False)
                for x in entries:
                    dest=a.extract.joinpath(*pathlib.PurePosixPath(x.filename).parts); dest.parent.mkdir(parents=True,exist_ok=True)
                    with z.open(x) as src, dest.open('xb') as dst:
                        import shutil
                        shutil.copyfileobj(src,dst)
        except Exception as ex: p.exit(2,'Extraction failed (partial directory may remain): '+str(ex)+'\n')
    a.out.parent.mkdir(parents=True,exist_ok=True); a.out.write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(r['summary'],ensure_ascii=False)); print('Report:',a.out)
    return 1 if r['errors'] else 0
if __name__=='__main__': raise SystemExit(main())
