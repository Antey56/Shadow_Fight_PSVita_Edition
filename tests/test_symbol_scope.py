import pathlib,sys,unittest,ctypes as C,struct
ROOT=pathlib.Path(__file__).parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import test_arm_link as al
DLL=next((p for p in [ROOT/'build-host/arm_link_host.dll',ROOT/'build-host/arm_link_host.so'] if p.exists()),ROOT/'build-host/arm_link_host.so')
class Entry(C.Structure):
    _fields_=[('provider',C.c_char_p),('name',C.c_char_p),('value',C.c_uint32),('type',C.c_uint),('binding',C.c_uint)]
class Scope(C.Structure):
    _fields_=[('entries',C.POINTER(Entry)),('count',C.c_size_t)]
@unittest.skipUnless(DLL.exists(),'build arm_link_host first')
class ScopeIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll=C.CDLL(str(DLL.resolve()))
        cls.init=staticmethod(cls.dll.arm_scope_init);cls.init.argtypes=[C.POINTER(Scope),C.POINTER(Entry),C.c_size_t];cls.init.restype=C.c_int
        cls.link=staticmethod(cls.dll.arm_link_image);cls.link.argtypes=[C.POINTER(C.c_ubyte),C.c_size_t,C.c_uint32,C.c_size_t,al.Resolver,C.c_void_p,C.POINTER(al.Image),C.POINTER(al.Error)];cls.link.restype=C.c_int
        cls.dispose=staticmethod(cls.dll.arm_link_dispose);cls.dispose.argtypes=[C.POINTER(al.Image)]
        cls.resolver=staticmethod(al.Resolver(('arm_scope_resolve',cls.dll)))
    def pipeline(self,entries,binding=1):
        array=(Entry*len(entries))(*(Entry(*e) for e in entries));scope=Scope()
        rc=self.init(C.byref(scope),array,len(array))
        if rc:return rc,None,b''
        b=al.external_fixture(binding);buf=(C.c_ubyte*len(b)).from_buffer_copy(b);out=al.Image();error=al.Error()
        rc=self.link(buf,len(b),0x10000000,4096,self.resolver,C.cast(C.byref(scope),C.c_void_p),C.byref(out),C.byref(error))
        result=struct.unpack_from('<I',C.string_at(out.bytes,out.size),320)[0] if out.bytes else None
        self.dispose(C.byref(out));return rc,result,error.missing_symbol
    def test_provider_resolves_actual_fixture(self):self.assertEqual(self.pipeline([(b'dep',b'foo',0x20000001,2,1)])[:2],(0,0x20000001))
    def test_first_weak_definition_wins(self):
        self.assertEqual(self.pipeline([(b'first',b'foo',0x1001,2,2),(b'second',b'foo',0x2001,2,1)])[:2],(0,0x1001))
    def test_explicit_reverse_order(self):
        self.assertEqual(self.pipeline([(b'second',b'foo',0x2001,2,1),(b'first',b'foo',0x1001,2,2)])[:2],(0,0x2001))
    def test_type_mismatch_is_error(self):
        self.assertEqual(self.pipeline([(b'data',b'foo',0x1000,1,1),(b'code',b'foo',0x2001,2,1)])[:2],(-21,None))
    def test_missing_required(self):self.assertEqual(self.pipeline([]),(-20,None,b'foo'))
    def test_missing_weak(self):self.assertEqual(self.pipeline([],binding=2)[:2],(0,0))
    def test_duplicate_provider_name(self):self.assertEqual(self.pipeline([(b'dep',b'foo',0x1001,2,1),(b'dep',b'foo',0x2001,2,1)])[0],-3)
    def test_zero_function_rejected(self):self.assertEqual(self.pipeline([(b'dep',b'foo',0,2,1)])[0],-2)
    def test_local_binding_not_exported(self):self.assertEqual(self.pipeline([(b'dep',b'foo',0x1001,2,0)])[0],-2)
