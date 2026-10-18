import pathlib,sys,struct,unittest,ctypes as C
ROOT=pathlib.Path(__file__).parents[1];sys.path.insert(0,str(ROOT/'tools'))
from verify_dynamic_c import load,read,RawRel,Info
from test_elf_dynamic import fixture
DLL=next((p for p in [ROOT/'build-host/arm_dynamic_host.dll',ROOT/'build-host/arm_dynamic_host.so'] if p.exists()),ROOT/'build-host/arm_dynamic_host.so')
@unittest.skipUnless(DLL.exists(),'build arm_dynamic_host.dll first')
class DynamicCTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.fn=staticmethod(load(DLL))
    def test_valid(self):
        rc,rels,info=read(self.fn,fixture());self.assertEqual(rc,0);self.assertEqual(rels,[(320,23,0)])
    def test_truncated(self):self.assertNotEqual(read(self.fn,fixture()[:100])[0],0)
    def test_unknown_type(self):
        b=fixture();struct.pack_into('<I',b,308,160)
        rc,_,info=read(self.fn,b);self.assertEqual(rc,-6);self.assertEqual(info.relocation_count,0)
    def test_symbol_bounds(self):
        b=fixture();struct.pack_into('<I',b,308,(1000<<8)|2)
        self.assertEqual(read(self.fn,b)[0],-10)
    def test_missing_terminator(self):
        b=fixture();struct.pack_into('<I',b,172,1)
        self.assertEqual(read(self.fn,b)[0],-5)
    def test_relative_symbol_rejected(self):
        b=fixture();struct.pack_into('<I',b,308,(1<<8)|23)
        self.assertEqual(read(self.fn,b)[0],-6)
    def test_capacity(self):
        b=fixture();buf=(C.c_ubyte*len(b)).from_buffer_copy(b);out=(RawRel*1)();info=Info()
        self.assertEqual(self.fn(buf,len(b),out,0,C.byref(info)),-8)
        self.assertEqual(info.relocation_count,0)
    def test_packed_rejected(self):
        b=fixture();struct.pack_into('<I',b,148,0x6000000f)
        self.assertEqual(read(self.fn,b)[0],-6)
