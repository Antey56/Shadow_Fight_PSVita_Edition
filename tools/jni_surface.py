#!/usr/bin/env python3
"""Static, non-executing JNI surface inventory. DEX references are not calls."""
import argparse
import hashlib
import json
import pathlib
import re
import struct
import zipfile
from analyze_apk import dex_strings, u32
from deep_audit import native_methods

# Only these framework/engine API families enter the published method inventory.
PREFIXES = ('Lcom/unity3d/player/', 'Landroid/content/', 'Landroid/app/',
            'Landroid/view/', 'Landroid/os/', 'Ljava/io/')
TOKENS = ('android/content/res/AssetManager', 'android/content/Context',
          'getAssets', 'getFilesDir', 'getCacheDir', 'getPackageCodePath',
          'getPackageName', 'getExternalFilesDir', 'open', 'openFd',
          'AAssetManager_fromJava', 'AAssetManager_open', 'JNI_OnLoad')
METHODS = frozenset(('getAssets', 'getFilesDir', 'getCacheDir', 'getPackageCodePath',
                     'getPackageName', 'getExternalFilesDir', 'getApplicationInfo',
                     'getResources', 'open', 'openFd', 'list', 'getObbDir'))


def method_references(data):
    strings = dex_strings(data)
    types = [strings[u32(data, u32(data, 68) + i*4)]
             for i in range(u32(data, 64))]
    protos = []
    for i in range(u32(data, 72)):
        _, ret, params = struct.unpack_from('<III', data, u32(data, 76)+i*12)
        args = [] if not params else [types[struct.unpack_from('<H', data, params+4+j*2)[0]]
                                     for j in range(u32(data, params))]
        protos.append('('+''.join(args)+')'+types[ret])
    out = []
    for i in range(u32(data, 88)):
        cls, proto, name = struct.unpack_from('<HHI', data, u32(data, 92)+i*8)
        if types[cls].startswith(PREFIXES) and strings[name] in METHODS:
            out.append({'class': types[cls], 'method': strings[name],
                        'descriptor': protos[proto]})
    return out


def inventory(apk):
    declarations, refs, evidence = [], [], []
    digest = hashlib.sha256()
    with apk.open('rb') as f:
        for chunk in iter(lambda: f.read(1024*1024), b''):
            digest.update(chunk)
    with zipfile.ZipFile(apk) as z:
        for name in z.namelist():
            if re.fullmatch(r'classes\d*\.dex', name):
                data = z.read(name)
                declarations.extend(dict(m, dex=name) for m in native_methods(data))
                refs.extend(dict(m, dex=name) for m in method_references(data))
            elif name.startswith('lib/armeabi-v7a/') and name.endswith('.so'):
                data = z.read(name)
                # Exact NUL-terminated ASCII strings only; no arbitrary strings copied.
                present = [t for t in TOKENS if b'\0'+t.encode()+b'\0' in data]
                evidence.append({'module': name, 'exact_string_candidates': present})
    return {'apk_sha256': digest.hexdigest(), 'native_declarations': declarations,
            'framework_method_references': refs, 'native_string_evidence': evidence,
            'game_executed': False, 'runtime_ready': False,
            'limitations': ['DEX method IDs include definitions and references, not an executed call graph.',
                           'String presence is not proof of a JNI call or receiver/signature pairing.',
                           'String absence does not exclude constructed names or dynamic lookup.',
                           'No JNIEnv, JavaVM, class objects or Android lifecycle are implemented.']}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('apk', type=pathlib.Path)
    p.add_argument('--out', type=pathlib.Path, required=True)
    a = p.parse_args()
    report = inventory(a.apk)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'native_declarations': len(report['native_declarations']),
                      'framework_method_references': len(report['framework_method_references']),
                      'native_modules': len(report['native_string_evidence'])}))
