import ctypes as C,pathlib,sys,struct,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from test_arm_link import external_fixture
class Export(C.Structure):_fields_=[('value',C.c_uint32),('size',C.c_uint32),('binding',C.c_uint),('type',C.c_uint),('absolute',C.c_uint)]
DLL=next((p for p in (ROOT/'build-host/arm_symbols_host.dll',ROOT/'build-host/arm_symbols_host.so') if p.exists()),ROOT/'build-host/arm_symbols_host.so')
def symbols():
    b=external_fixture();struct.pack_into('<I',b,76,5)
    struct.pack_into('<I',b,292,401);struct.pack_into('<I',b,296,4);struct.pack_into('<H',b,302,1)
    struct.pack_into('<II',b,172,4,352);struct.pack_into('<II',b,180,0,0);struct.pack_into('<II',b,100,72,72)
    struct.pack_into('<IIIII',b,352,1,2,1,0,0)
    return b
@unittest.skipUnless(DLL.exists(),'build arm_symbols_host first')
class SymbolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib=C.CDLL(str(DLL.resolve()));cls.lib.arm_symbol_find.argtypes=[C.c_void_p,C.c_size_t,C.c_char_p,C.c_uint32,C.POINTER(Export)]
    def find(self,b=None,name=b'foo',bias=0x10000000):
        b=symbols() if b is None else b;buf=C.create_string_buffer(bytes(b));out=Export()
        rc=self.lib.arm_symbol_find(buf,len(b),name,bias,C.byref(out));return rc,out
    def test_function_thumb_and_bias(self):
        rc,s=self.find();self.assertEqual(rc,1);self.assertEqual(s.value,0x10000191);self.assertEqual(s.type,2)
    def test_absent(self):
        rc,s=self.find(name=b'absent');self.assertEqual(rc,0);self.assertEqual(s.value,0)
    def test_undefined_not_export(self):
        b=symbols();struct.pack_into('<H',b,302,0);self.assertEqual(self.find(b)[0],0)
    def test_hidden_and_local_not_exported(self):
        b=symbols();b[301]=2;self.assertEqual(self.find(b)[0],0)
        b=symbols();b[300]=2;self.assertEqual(self.find(b)[0],0)
    def test_weak_protected(self):
        b=symbols();b[300]=0x22;b[301]=3;rc,s=self.find(b)
        self.assertEqual(rc,1);self.assertEqual(s.binding,2)
    def test_absolute_not_rebased(self):
        b=symbols();struct.pack_into('<H',b,302,0xfff1);rc,s=self.find(b)
        self.assertEqual(rc,1);self.assertEqual(s.value,401);self.assertEqual(s.absolute,1)
    def test_bad_name(self):
        b=symbols();struct.pack_into('<I',b,288,16);self.assertLess(self.find(b)[0],0)
    def test_bad_hash_count_or_pointer(self):
        b=symbols();struct.pack_into('<I',b,356,1000001);self.assertLess(self.find(b)[0],0)
        b=symbols();struct.pack_into('<I',b,176,0xfffffffc);self.assertLess(self.find(b)[0],0)
    def test_ifunc_and_tls_rejected(self):
        for typ in (6,10):
            b=symbols();b[300]=0x10|typ;self.assertEqual(self.find(b)[0],-64)
    def test_function_outside_code(self):
        b=symbols();struct.pack_into('<I',b,292,1001);self.assertEqual(self.find(b)[0],-65)
    def test_modular_bias(self):
        rc,s=self.find(bias=0xffffff00);self.assertEqual(rc,1);self.assertEqual(s.value,145)

    def versioned(self,hidden=False):
        b=symbols();b[261:269]=b'default\0'
        for off,tag,val in [(180,0x6ffffff0,376),(188,0x6ffffffc,392),(196,0x6ffffffd,1),(204,0,0)]:
            struct.pack_into('<II',b,off,tag,val)
        struct.pack_into('<II',b,100,96,96);struct.pack_into('<HH',b,376,0,0x8002 if hidden else 2)
        struct.pack_into('<HHHHIII',b,392,1,0,2,1,0,20,0);struct.pack_into('<II',b,412,5,0)
        return b
    def test_default_version_and_nondefault_hidden(self):
        self.assertEqual(self.find(self.versioned())[0],1)
        self.assertEqual(self.find(self.versioned(True))[0],0)
    def test_unknown_version_and_malformed_definition(self):
        b=self.versioned();struct.pack_into('<H',b,378,3);self.assertEqual(self.find(b)[0],-66)
        b=self.versioned();struct.pack_into('<I',b,404,0xfffffffe);self.assertEqual(self.find(b)[0],-66)

    def test_arm_function_alignment(self):
        b=symbols();struct.pack_into('<I',b,292,402);self.assertEqual(self.find(b)[0],-65)
