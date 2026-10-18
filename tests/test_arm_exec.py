import ctypes as C
import pathlib,struct,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from test_elf_dynamic import fixture
from test_arm_link import Resolver,Error,external_fixture
P=C.c_void_p;U=C.c_uint32;Z=C.c_size_t
Reserve=C.CFUNCTYPE(C.c_int,P,Z,Z,C.POINTER(U),C.POINTER(Z))
Publish=C.CFUNCTYPE(C.c_int,P,Z,P,Z)
Sync=C.CFUNCTYPE(C.c_int,P,Z,Z)
Release=C.CFUNCTYPE(None,P,Z)
class Backend(C.Structure):_fields_=[('context',P),('reserve',Reserve),('publish',Publish),('sync',Sync),('release',Release)]
class Range(C.Structure):_fields_=[('begin',U),('end',U)]
class Image(C.Structure):_fields_=[('backend',Backend),('cookie',Z),('base',U),('bias',U),('size',Z),('code',Range*8),('code_count',C.c_uint),('ready',C.c_int)]
DLL=next((p for p in (ROOT/'build-host/arm_exec_host.dll',ROOT/'build-host/arm_exec_host.so') if p.exists()),ROOT/'build-host/arm_exec_host.so')
def executable():
    b=fixture();struct.pack_into('<I',b,76,5);return b
@unittest.skipUnless(DLL.exists(),'build arm_exec_host first')
class ExecTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib=C.CDLL(str(DLL.resolve()))
        cls.lib.arm_exec_map.argtypes=[P,Z,Z,Resolver,P,C.POINTER(Backend),C.POINTER(Image),C.POINTER(Error)]
        cls.lib.arm_exec_entry.argtypes=[C.POINTER(Image),U,C.POINTER(U)]
        cls.lib.arm_exec_release.argtypes=[C.POINTER(Image)]
    def run_map(self,b=None,fail=None,address=0x10000000,resolve=None,limit=4096):
        self.events=[];self.published=None
        def reserve(ctx,n,align,base,cookie):
            self.events.append('reserve');base[0]=address;cookie[0]=42
            return -50 if fail=='reserve' else 0
        def publish(ctx,cookie,p,n):
            self.events.append('publish');self.published=C.string_at(p,n)
            return -51 if fail=='publish' else 0
        def sync(ctx,cookie,n):self.events.append('sync');return -52 if fail=='sync' else 0
        def release(ctx,cookie):self.events.append('release')
        self.callbacks=[Reserve(reserve),Publish(publish),Sync(sync),Release(release)]
        backend=Backend(None,*self.callbacks);self.image=Image();error=Error()
        self.resolver=Resolver(resolve or (lambda *args:0));b=executable() if b is None else b
        buf=C.create_string_buffer(bytes(b))
        self.rc=self.lib.arm_exec_map(buf,len(b),limit,self.resolver,None,C.byref(backend),C.byref(self.image),C.byref(error))
        return self.rc
    def dispose(self):self.lib.arm_exec_release(C.byref(self.image))
    def test_publish_sync_then_ready(self):
        self.assertEqual(self.run_map(),0);self.assertTrue(self.image.ready)
        self.assertEqual(self.events,['reserve','publish','sync'])
        self.assertEqual(struct.unpack_from('<I',self.published,320)[0],0x10000000)
        self.dispose();self.dispose();self.assertEqual(self.events.count('release'),1)
    def test_reserve_failure_no_release(self):
        self.assertEqual(self.run_map(fail='reserve'),-50);self.assertFalse(self.image.ready)
        self.assertEqual(self.events,['reserve'])
    def test_publish_failure_releases_without_sync(self):
        self.assertEqual(self.run_map(fail='publish'),-51);self.assertFalse(self.image.ready)
        self.assertEqual(self.events,['reserve','publish','release'])
    def test_sync_failure_releases(self):
        self.assertEqual(self.run_map(fail='sync'),-52);self.assertFalse(self.image.ready)
        self.assertEqual(self.events,['reserve','publish','sync','release'])
    def test_missing_import_never_published(self):
        b=external_fixture();struct.pack_into('<I',b,76,5)
        self.assertEqual(self.run_map(b),-20);self.assertEqual(self.events,['reserve','release'])
    def test_no_executable_segment(self):
        self.assertEqual(self.run_map(fixture()),-32);self.assertEqual(self.events,[])
    def test_writable_executable_rejected(self):
        b=executable();struct.pack_into('<I',b,76,7)
        self.assertEqual(self.run_map(b),-32);self.assertEqual(self.events,[])
    def test_small_limit_and_misaligned_base(self):
        self.assertEqual(self.run_map(limit=100),-31);self.assertEqual(self.events,[])
        self.assertEqual(self.run_map(address=0x10000001),-33);self.assertEqual(self.events,['reserve','release'])
    def test_constructor_requires_future_runtime(self):
        b=executable();struct.pack_into('<II',b,172,12,400);struct.pack_into('<II',b,180,0,0)
        struct.pack_into('<II',b,100,72,72)
        self.assertEqual(self.run_map(b),-32);self.assertEqual(self.events,[])
    def test_tls_requires_future_runtime(self):
        b=executable();headers=bytes(b[52:116]);struct.pack_into('<I',b,28,384);struct.pack_into('<H',b,44,3)
        b[384:448]=headers;struct.pack_into('<IIIIIIII',b,448,7,0,0,0,0,16,4,4)
        self.assertEqual(self.run_map(b),-32);self.assertEqual(self.events,[])
    def test_entry_state_alignment_and_bounds(self):
        self.assertEqual(self.run_map(),0);out=U()
        self.assertEqual(self.lib.arm_exec_entry(C.byref(self.image),0x181,C.byref(out)),0)
        self.assertEqual(out.value,0x10000181)
        for ptr in [0x182,0x201,0xffffffff]:
            self.assertNotEqual(self.lib.arm_exec_entry(C.byref(self.image),ptr,C.byref(out)),0);self.assertEqual(out.value,0)
        self.dispose();self.assertNotEqual(self.lib.arm_exec_entry(C.byref(self.image),0x181,C.byref(out)),0)
    def test_bss_is_not_executable_entry(self):
        b=executable();struct.pack_into('<I',b,72,768)
        self.assertEqual(self.run_map(b),0);out=U()
        self.assertNotEqual(self.lib.arm_exec_entry(C.byref(self.image),0x201,C.byref(out)),0);self.dispose()

    def test_entry_with_modular_negative_load_bias(self):
        image=Image();image.ready=1;image.base=0x1000;image.bias=0xffff1000
        image.code_count=1;image.code[0].begin=0x1000;image.code[0].end=0x1200
        out=U();self.assertEqual(self.lib.arm_exec_entry(C.byref(image),0x10181,C.byref(out)),0)
        self.assertEqual(out.value,0x1181)
        self.assertNotEqual(self.lib.arm_exec_entry(C.byref(image),0x181,C.byref(out)),0)
