#!/usr/bin/env python3
"""Package authored sources/reports only; exclude APKs, extracted libs and binaries."""
from pathlib import Path
import hashlib,zipfile
root=Path(__file__).resolve().parents[1]
allowed={'tools','tests','vita','docs','reports','.github','licenses','artifacts'}
files=[p for p in root.rglob('*') if p.is_file() and not p.is_symlink() and '__pycache__' not in p.parts and (p.relative_to(root).parts[0] in allowed or p.parent==root) and p.suffix.lower() not in {'.exe','.pyc','.so','.apk','.apkm','.apks','.zip'} and p.name!='CHECKSUMS.sha256']
checksums=root/'CHECKSUMS.sha256'
checksums.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(root).as_posix()+'\n' for p in sorted(files)),encoding='utf-8')
files.append(checksums)
out=root.parent/'shadow-fight-2-special-1.0.12-vita-research.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(files):z.write(p,root.name+'/'+p.relative_to(root).as_posix())
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    assert not any('/local-input/' in n or n.endswith(('.apk','.so','.exe')) for n in z.namelist())
print(out, out.stat().st_size,'bytes,',len(files),'files')
