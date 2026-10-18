#!/usr/bin/env python3
"""Create an allowlisted publication stage; never include SDK/game binaries."""
from pathlib import Path
import hashlib,json
from public_report import public_report
root=Path(__file__).resolve().parents[1]
stage=root.parent/'github-public-stage'
allowed={'tools','tests','vita','docs','reports','.github','licenses','artifacts'}
files=[]
for p in sorted(root.rglob('*')):
    if not p.is_file() or p.is_symlink() or '__pycache__' in p.parts:continue
    rel=p.relative_to(root)
    if rel.parts[0] not in allowed and p.parent!=root:continue
    if p.suffix.lower() in {'.exe','.pyc','.so','.dll','.o','.obj','.a','.apk','.apkm','.apks','.zip','.gz'}:continue
    if p.name=='CHECKSUMS.sha256':continue
    if p.suffix=='.vpk' and rel.as_posix()!='artifacts/sf2-diagnostic.vpk':raise ValueError('unexpected VPK')
    data=p.read_bytes();name=rel.as_posix()
    if p.suffix not in {'.png','.vpk'}:data=data.replace(b'\r\n',b'\n')
    if p.suffix=='.json' and len(data)>128*1024:
        report=json.loads(data)
        if not all(k in report for k in ('libraries','manifest','apk','summary','dex','errors')):raise ValueError('Unreviewed large JSON')
        data=json.dumps(public_report(report),ensure_ascii=False,indent=2).encode('utf-8')
    dest=stage/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
    files.append({'path':name,'size':len(data),'binary':dest.suffix in {'.png','.vpk'},'sha256':hashlib.sha256(data).hexdigest()})
checksums=''.join(x['sha256']+'  '+x['path']+'\n' for x in files)
(stage/'CHECKSUMS.sha256').write_bytes(checksums.encode('utf-8'))
files.append({'path':'CHECKSUMS.sha256','size':len(checksums),'binary':False})
(stage.parent/'github-publish-index.json').write_text(json.dumps(files,indent=2),encoding='utf-8')
print('Staged',len(files),'files;',sum(x['size'] for x in files),'bytes; stage:',stage)
