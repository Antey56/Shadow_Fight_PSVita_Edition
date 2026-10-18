#!/usr/bin/env python3
"""Convert our small-palette RGB(A) PNG assets losslessly to indexed PNG8."""
import argparse
import pathlib
import struct
import zlib
import xml.etree.ElementTree as ET
SIGNATURE=b'\x89PNG\r\n\x1a\n'
ASSETS={'icon0.png':(128,128),'livearea/contents/bg.png':(840,500),
        'livearea/contents/startup.png':(280,158)}

def chunks(data):
    if not data.startswith(SIGNATURE):raise ValueError('not PNG')
    off=8
    while off<len(data):
        if off+12>len(data):raise ValueError('truncated PNG')
        n=struct.unpack_from('>I',data,off)[0];tag=data[off+4:off+8]
        if off+n+12>len(data):raise ValueError('truncated chunk')
        content=data[off+8:off+8+n];crc=struct.unpack_from('>I',data,off+8+n)[0]
        if zlib.crc32(tag+content)&0xffffffff!=crc:raise ValueError('PNG CRC mismatch')
        yield tag,content;off+=n+12
        if tag==b'IEND':
            if off!=len(data):raise ValueError('trailing PNG bytes')
            return
    raise ValueError('missing IEND')

def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data)&0xffffffff)

def png_info(data):
    cs=list(chunks(data))
    if not cs or cs[0][0]!=b'IHDR':raise ValueError('missing IHDR')
    return struct.unpack('>IIBBBBB',cs[0][1]),cs

def paeth(a,b,c):
    p=a+b-c;da,db,dc=abs(p-a),abs(p-b),abs(p-c)
    return a if da<=db and da<=dc else b if db<=dc else c

def indexed(data,expected):
    (w,h,depth,color,compression,filtering,interlace),cs=png_info(data)
    if (w,h)!=expected or w*h>1024*1024:raise ValueError('wrong LiveArea dimensions')
    if depth!=8 or compression or filtering or interlace:raise ValueError('requires noninterlaced 8-bit PNG')
    if color==3:
        palettes=[c for t,c in cs if t==b'PLTE']
        if len(palettes)!=1 or not 0<len(palettes[0])<=768 or len(palettes[0])%3:raise ValueError('invalid palette')
        raw=zlib.decompress(b''.join(c for t,c in cs if t==b'IDAT'))
        if len(raw)!=(w+1)*h or any(raw[y*(w+1)]>4 for y in range(h)):raise ValueError('invalid indexed scanlines')
        return data
    if color not in (2,6):raise ValueError('requires RGB/RGBA or indexed PNG')
    bpp=3 if color==2 else 4;stride=w*bpp
    raw=zlib.decompress(b''.join(c for t,c in cs if t==b'IDAT'))
    if len(raw)!=(stride+1)*h:raise ValueError('PNG scanline length mismatch')
    previous=bytearray(stride);pixels=bytearray();palette=[];lookup={}
    for y in range(h):
        start=y*(stride+1);ft=raw[start];row=bytearray(raw[start+1:start+1+stride])
        if ft>4:raise ValueError('invalid PNG filter')
        if ft:
            for x in range(stride):
                left=row[x-bpp] if x>=bpp else 0;above=previous[x];corner=previous[x-bpp] if x>=bpp else 0
                predictor=left if ft==1 else above if ft==2 else (left+above)//2 if ft==3 else paeth(left,above,corner)
                row[x]=(row[x]+predictor)&255
        pixels.append(0)
        for x in range(0,stride,bpp):
            rgba=tuple(row[x:x+3])+(row[x+3] if bpp==4 else 255,)
            if rgba not in lookup:
                if len(palette)==256:raise ValueError('more than 256 colors: quantize with pngquant/Pillow before packaging')
                lookup[rgba]=len(palette);palette.append(rgba)
            pixels.append(lookup[rgba])
        previous=row
    pal=b''.join(bytes(c[:3]) for c in palette).ljust(768,b'\0')
    result=SIGNATURE+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,3,0,0,0))+chunk(b'PLTE',pal)
    if any(c[3]!=255 for c in palette):result+=chunk(b'tRNS',bytes(c[3] for c in palette))
    return result+chunk(b'IDAT',zlib.compress(bytes(pixels),9))+chunk(b'IEND',b'')

def validate_xml(data):
    root=ET.fromstring(data)
    if root.tag!='livearea' or root.get('style')!='a1' or root.get('format-ver')!='01.00':raise ValueError('unsupported LiveArea root/schema')
    if root.findtext('livearea-background/image')!='bg.png' or root.findtext('gate/startup-image')!='startup.png':raise ValueError('unexpected LiveArea image references')
    return root

def prepare(source,out):
    for relative,dimensions in ASSETS.items():
        target=out/relative;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(indexed((source/relative).read_bytes(),dimensions))
    xml=(source/'livearea/contents/template.xml').read_bytes();validate_xml(xml)
    (out/'livearea/contents/template.xml').write_bytes(xml)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
    a=p.parse_args();prepare(a.source,a.out);print('LiveArea PNG8 and XML prepared')
