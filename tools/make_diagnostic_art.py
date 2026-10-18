#!/usr/bin/env python3
"""Generate original, dependency-free placeholder LiveArea art for diagnostics."""
from pathlib import Path
import struct,zlib
root=Path(__file__).resolve().parents[1]/'vita/sce_sys'
glyphs={'S':[70,73,73,73,49],'F':[127,9,9,9,1],'2':[98,81,73,73,70]}
def png(path,w,h,scale):
    pixels=bytearray(bytes([12,20,34,255])*(w*h))
    def rect(x,y,rw,rh,color):
        for yy in range(max(0,y),min(h,y+rh)):
            for xx in range(max(0,x),min(w,x+rw)):pixels[(yy*w+xx)*4:(yy*w+xx)*4+4]=bytes(color)
    rect(0,0,w,max(4,h//25),(38,204,161,255))
    x=(w-17*scale)//2;y=(h-7*scale)//2
    for ch in 'SF2':
        for col,bits in enumerate(glyphs[ch]):
            for row in range(7):
                if bits&(1<<row):rect(x+col*scale,y+row*scale,scale,scale,(220,230,245,255))
        x+=6*scale
    def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xffffffff)
    raw=b''.join(b'\0'+pixels[y*w*4:(y+1)*w*4] for y in range(h))
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
png(root/'icon0.png',128,128,6)
png(root/'livearea/contents/bg.png',840,500,20)
png(root/'livearea/contents/startup.png',280,158,11)
