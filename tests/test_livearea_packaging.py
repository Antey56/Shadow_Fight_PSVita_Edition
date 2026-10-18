import pathlib
import struct
import sys
import unittest
import zlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from prepare_livearea import SIGNATURE,chunk,indexed,png_info,validate_xml
from validate_vpk import sfo_fields

def png(color=6,filter_type=0):
    bpp=4 if color==6 else 3
    row=bytes([20,40,60,255][:bpp])*2
    previous=0;encoded=bytearray()
    for i,v in enumerate(row):
        left=row[i-bpp] if i>=bpp else 0
        predictor=left if filter_type in (1,4) else left//2 if filter_type==3 else 0
        encoded.append((v-predictor)&255)
    return SIGNATURE+chunk(b'IHDR',struct.pack('>IIBBBBB',2,1,8,color,0,0,0))+chunk(b'IDAT',zlib.compress(bytes([filter_type])+encoded))+chunk(b'IEND',b'')

class PackagingTests(unittest.TestCase):
    def test_rgba_is_palette8(self):
        result=indexed(png(),(2,1));info,parts=png_info(result)
        self.assertEqual(info[2:4],(8,3));self.assertTrue(any(t==b'PLTE' for t,_ in parts))
    def test_rgb_and_all_filters(self):
        for color in (2,6):
            for f in range(5):
                with self.subTest(color=color,filter=f):
                    _,cs=png_info(indexed(png(color,f),(2,1)))
                    self.assertEqual(next(c for t,c in cs if t==b'PLTE')[:3],bytes([20,40,60]))
                    self.assertEqual(zlib.decompress(next(c for t,c in cs if t==b'IDAT')),b'\0\0\0')
    def test_already_palette_unchanged(self):
        result=indexed(png(),(2,1));self.assertEqual(indexed(result,(2,1)),result)
    def test_crc_failure(self):
        b=bytearray(png());b[-1]^=1
        with self.assertRaisesRegex(ValueError,'CRC'):indexed(bytes(b),(2,1))
    def test_dimension_failure(self):
        with self.assertRaisesRegex(ValueError,'dimensions'):indexed(png(),(128,128))
    def test_transparency_preserved(self):
        raw=b'\0\x14\x28\x3c\0\x14\x28\x3c\xff'
        b=SIGNATURE+chunk(b'IHDR',struct.pack('>IIBBBBB',2,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')
        _,cs=png_info(indexed(b,(2,1)));self.assertEqual(next(c for t,c in cs if t==b'tRNS'),b'\0\xff')
    def test_xml_schema(self):
        validate_xml(b'<livearea style="a1" format-ver="01.00"><livearea-background><image>bg.png</image></livearea-background><gate><startup-image>startup.png</startup-image></gate></livearea>')
    def test_xml_wrong_gate_rejected(self):
        with self.assertRaises(ValueError):validate_xml(b'<livearea style="a1" format-ver="01.00"><gate><image>startup.png</image></gate></livearea>')
    def test_malformed_xml_rejected(self):
        with self.assertRaises(Exception):validate_xml(b'<livearea>')
    def test_sfo_version_read(self):
        keys=b'TITLE_ID\0APP_VER\0';values=b'SF2AUD001\0' + b'00.07\0'
        base=20+32+len(keys)
        b=struct.pack('<4sIIII',b'\0PSF',0x101,52,base,2)
        b+=struct.pack('<HHIII',0,0x204,10,10,0)+struct.pack('<HHIII',9,0x204,6,6,10)+keys+values
        self.assertEqual(sfo_fields(b),{'TITLE_ID':'SF2AUD001','APP_VER':'00.07'})
