#!/usr/bin/env python3
"""Reject bad LiveArea resources and unexpected diagnostic SFO before delivery."""
import argparse
import hashlib
import json
import pathlib
import struct
import zipfile
from prepare_livearea import ASSETS,indexed,png_info,validate_xml

def sfo_fields(b):
    magic,version,keys,values,count=struct.unpack_from('<4sIIII',b)
    if magic!=b'\0PSF' or count>128:raise ValueError('invalid SFO')
    fields={}
    for i in range(count):
        key,fmt,size,maximum,value=struct.unpack_from('<HHIII',b,20+i*16)
        if size>maximum or values+value+size>len(b):raise ValueError('invalid SFO bounds')
        end=b.find(b'\0',keys+key,values)
        if end<0:raise ValueError('SFO key terminator missing')
        name=b[keys+key:end].decode('ascii');raw=b[values+value:values+value+size]
        if fmt==0x204:fields[name]=raw.rstrip(b'\0').decode('utf-8')
    return fields

def validate(path,version):
    result={'file':path.name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'pngs':[]}
    with zipfile.ZipFile(path) as z:
        if z.testzip():raise ValueError('VPK ZIP CRC failure')
        if len(z.namelist())!=len(set(z.namelist())):raise ValueError('duplicate ZIP names')
        for relative,dimensions in ASSETS.items():
            name='sce_sys/'+relative;data=z.read(name)
            (w,h,depth,color,*_),_=png_info(data)
            if depth!=8 or color!=3:raise ValueError(name+' must be indexed PNG8')
            indexed(data,dimensions);result['pngs'].append({'path':name,'width':w,'height':h,'bit_depth':depth,'color_type':color})
        validate_xml(z.read('sce_sys/livearea/contents/template.xml'))
        fields=sfo_fields(z.read('sce_sys/param.sfo'))
        if fields.get('TITLE_ID')!='SF2AUD001' or fields.get('APP_VER')!=version:raise ValueError('unexpected diagnostic SFO identity/version')
        if not z.read('eboot.bin').startswith(b'SCE\0'):raise ValueError('missing SELF header')
        result.update(title_id=fields['TITLE_ID'],app_version=fields['APP_VER'],xml_checked=True,zip_crc_checked=True)
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('vpk',type=pathlib.Path);p.add_argument('--version',required=True);p.add_argument('--out',type=pathlib.Path)
    a=p.parse_args();r=validate(a.vpk,a.version)
    if a.out:a.out.write_text(json.dumps(r,indent=2),encoding='utf-8')
    print(json.dumps(r))
