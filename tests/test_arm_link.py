import pathlib,sys,struct,ctypes as C,unittest
ROOT=pathlib.Path(__file__).parents[1];sys.path.insert(0,str(ROOT/'tools'))
from test_elf_dynamic import fixture
DLL=next((p for p in [ROOT/'build-host/arm_link_host.dll',ROOT/'build-host/arm_link_host.so'] if p.exists()),ROOT/'build-host/arm_link_host.so')
class Image(C.Structure):
    _fields_=[('bytes',C.POINTER(C.c_ubyte)),('size',C.c_size_t),('vaddr_min',C.c_uint32),('bias',C.c_uint32)]
class Error(C.Structure):
    _fields_=[('code',C.c_int),('relocation_index',C.c_size_t),('missing_symbol',C.c_char*128)]
Resolver=C.CFUNCTYPE(C.c_int,C.c_char_p,C.c_uint,C.c_uint,C.POINTER(C.c_uint32),C.c_void_p)
def missing(name,bind,typ,value,ctx):return 0
def external_fixture(binding=1):
    b=fixture();b[256:261]=b'\0foo\0'
    struct.pack_into('<IIIBBH',b,288,1,0,0,(binding<<4)|2,0,0)
    struct.pack_into('<II',b,304,320,(1<<8)|22)
    return b
@unittest.skipUnless(DLL.exists(),'build arm_link_host first')
class LinkTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll=C.CDLL(str(DLL.resolve()));cls.link=staticmethod(cls.dll.arm_link_image)
        cls.link.argtypes=[C.POINTER(C.c_ubyte),C.c_size_t,C.c_uint32,C.c_size_t,Resolver,C.c_void_p,C.POINTER(Image),C.POINTER(Error)];cls.link.restype=C.c_int
        cls.dispose=staticmethod(cls.dll.arm_link_dispose);cls.dispose.argtypes=[C.POINTER(Image)]
    def execute(self,b,resolver=missing,start=0x10000000,limit=4096):
        buf=(C.c_ubyte*len(b)).from_buffer_copy(b);image=Image();error=Error();cb=Resolver(resolver)
        rc=self.link(buf,len(b),start,limit,cb,None,C.byref(image),C.byref(error))
        data=C.string_at(image.bytes,image.size) if image.bytes else None
        self.dispose(C.byref(image));self.assertFalse(image.bytes)
        return rc,data,error
    def test_relative_pipeline(self):
        b=fixture();struct.pack_into('<I',b,320,12)
        rc,data,_=self.execute(b);self.assertEqual(rc,0);self.assertEqual(struct.unpack_from('<I',data,320)[0],0x1000000c)
    def test_required_missing_no_image(self):
        rc,data,e=self.execute(external_fixture());self.assertEqual(rc,-20);self.assertIsNone(data);self.assertEqual(e.missing_symbol,b'foo')
    def test_resolver(self):
        def resolve(name,bind,typ,value,ctx):value[0]=0x20000001;return 1
        rc,data,_=self.execute(external_fixture(),resolve);self.assertEqual(rc,0);self.assertEqual(struct.unpack_from('<I',data,320)[0],0x20000001)
    def test_weak_missing(self):
        rc,data,_=self.execute(external_fixture(2));self.assertEqual(rc,0);self.assertEqual(struct.unpack_from('<I',data,320)[0],0)
    def test_local_defined(self):
        b=external_fixture(0);struct.pack_into('<I',b,292,401);struct.pack_into('<H',b,302,1)
        rc,data,_=self.execute(b);self.assertEqual(rc,0);self.assertEqual(struct.unpack_from('<I',data,320)[0],0x10000191)
    def test_small_budget(self):self.assertEqual(self.execute(fixture(),limit=100)[0],-11)
    def test_runtime_overflow(self):self.assertEqual(self.execute(fixture(),start=0xffffff00)[0],-11)
    def test_misaligned_bias(self):self.assertEqual(self.execute(fixture(),start=0x10000001)[0],-13)
    def test_resolver_error(self):
        self.assertEqual(self.execute(external_fixture(),lambda *args:-1)[0],-21)
    def test_invalid_definition(self):
        b=external_fixture(0);struct.pack_into('<I',b,292,10000);struct.pack_into('<H',b,302,1)
        self.assertEqual(self.execute(b)[0],-14)

    def test_notype_import_preserves_thumb_callback(self):
        b=external_fixture();b[300]=0x10
        def resolver(name,bind,typ,value,ctx):
            self.assertEqual(typ,0);value[0]=0x20000001;return 1
        rc,data,_=self.execute(b,resolver)
        self.assertEqual(rc,0);self.assertEqual(struct.unpack_from('<I',data,320)[0],0x20000001)
