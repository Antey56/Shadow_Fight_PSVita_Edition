import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from scan_native_tables import scan
from test_elf_dynamic import fixture

DECL=[{'method':'nativeRender','descriptor':'()Z','class':'Ltest/Player;'}]


def table_fixture():
    b=fixture()
    struct.pack_into('<I',b,52+24,7) # one executable LOAD for synthetic fixture
    struct.pack_into('<I',b,116+5*8+4,24) # DT_RELSZ
    for i in range(3):struct.pack_into('<II',b,304+8*i,352+4*i,23)
    struct.pack_into('<III',b,352,400,420,449)
    b[400:413]=b'nativeRender\0';b[420:424]=b'()Z\0'
    return b


class NativeTableTests(unittest.TestCase):
    def test_thumb(self):
        r=scan(table_fixture(),DECL)
        self.assertEqual(len(r),1);self.assertEqual(r[0]['code_vaddr'],448)
        self.assertEqual(r[0]['instruction_set'],'Thumb')
        self.assertFalse(r[0]['class_binding_confirmed'])

    def test_missing_relocation_rejected(self):
        b=table_fixture();struct.pack_into('<I',b,116+5*8+4,16)
        self.assertEqual(scan(b,DECL),[])

    def test_non_executable_rejected(self):
        b=table_fixture();struct.pack_into('<I',b,52+24,6)
        self.assertEqual(scan(b,DECL),[])

    def test_wrong_descriptor_rejected(self):
        b=table_fixture();b[422]=ord('V')
        self.assertEqual(scan(b,DECL),[])

    def test_unmapped_pointer_rejected(self):
        b=table_fixture();struct.pack_into('<I',b,360,4097)
        self.assertEqual(scan(b,DECL),[])

    def test_ambiguous_class_retained(self):
        d=DECL+[dict(DECL[0],**{'class':'Lother/Player;'})]
        self.assertEqual(len(scan(table_fixture(),d)[0]['class_candidates']),2)

    def test_arm_alignment_rejected(self):
        b=table_fixture();struct.pack_into('<I',b,360,450)
        self.assertEqual(scan(b,DECL),[])

    def test_arm(self):
        b=table_fixture();struct.pack_into('<I',b,360,448)
        self.assertEqual(scan(b,DECL)[0]['instruction_set'],'ARM')
