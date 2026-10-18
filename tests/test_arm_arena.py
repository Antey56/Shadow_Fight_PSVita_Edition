import ctypes as C
import unittest
from test_arm_exec import DLL,Backend,Image,Resolver,Error,executable
P=C.c_void_p;Z=C.c_size_t;U=C.c_uint32
Write=C.CFUNCTYPE(C.c_int,P,Z,P,Z)
Sync=C.CFUNCTYPE(C.c_int,P,Z,Z)
class Storage(C.Structure):
    _fields_=[('context',P),('write',Write),('sync',Sync)]
class Slot(C.Structure):
    _fields_=[('offset',U),('span',U),('requested',U),('generation',U),('active',C.c_uint),('phase',C.c_uint)]
class Arena(C.Structure):
    _fields_=[('base',U),('capacity',U),('storage',Storage),('slots',Slot*64)]
@unittest.skipUnless(DLL.exists(),'build arm_exec_host first')
class ArenaIntegration(unittest.TestCase):
    def setUp(self):
        self.lib=C.CDLL(str(DLL.resolve()))
        self.lib.arm_arena_init.argtypes=[C.POINTER(Arena),U,Z,C.POINTER(Storage)]
        self.lib.arm_arena_backend.argtypes=[C.POINTER(Arena)];self.lib.arm_arena_backend.restype=Backend
        self.lib.arm_arena_used.argtypes=[C.POINTER(Arena)];self.lib.arm_arena_used.restype=Z
        self.lib.arm_exec_map.argtypes=[P,Z,Z,Resolver,P,C.POINTER(Backend),C.POINTER(Image),C.POINTER(Error)]
        self.lib.arm_exec_release.argtypes=[C.POINTER(Image)]
        self.fail=None;self.memory=bytearray(16384);self.events=[]
        def write(ctx,off,source,size):
            self.events.append(('write',off,size))
            if self.fail=='write':return -71
            self.memory[off:off+size]=C.string_at(source,size);return 0
        def sync(ctx,off,size):
            self.events.append(('sync',off,size));return -72 if self.fail=='sync' else 0
        self.callbacks=[Write(write),Sync(sync)];storage=Storage(None,*self.callbacks)
        self.arena=Arena();self.assertEqual(self.lib.arm_arena_init(C.byref(self.arena),0x90000000,len(self.memory),C.byref(storage)),0)
        self.backend=self.lib.arm_arena_backend(C.byref(self.arena));self.images=[]
        self.resolver=Resolver(lambda *args:0)
    def map(self,data=None):
        data=executable() if data is None else data;buf=C.create_string_buffer(bytes(data))
        image=Image();error=Error()
        rc=self.lib.arm_exec_map(buf,len(data),len(self.memory),self.resolver,None,C.byref(self.backend),C.byref(image),C.byref(error))
        self.images.append(image);return rc,image
    def tearDown(self):
        for image in self.images:self.lib.arm_exec_release(C.byref(image))
        self.assertEqual(self.lib.arm_arena_used(C.byref(self.arena)),0)
    def test_two_live_images_preserve_relocations_and_contents(self):
        import struct
        rc,first=self.map();self.assertEqual(rc,0);saved=bytes(self.memory[:512])
        rc,second=self.map();self.assertEqual(rc,0)
        self.assertEqual(first.base,0x90000000);self.assertEqual(second.base,0x90001000)
        self.assertEqual(bytes(self.memory[:512]),saved)
        self.assertEqual(struct.unpack_from('<I',self.memory,320)[0],first.base)
        self.assertEqual(struct.unpack_from('<I',self.memory,4096+320)[0],second.base)
        self.assertEqual(self.lib.arm_arena_used(C.byref(self.arena)),8192)
        self.assertEqual([e[0] for e in self.events],['write','sync','write','sync'])
    def test_failed_publish_releases_only_failed_image(self):
        rc,first=self.map();self.assertEqual(rc,0);saved=bytes(self.memory[:512])
        self.fail='write';rc,second=self.map();self.assertEqual(rc,-71);self.assertFalse(second.ready)
        self.assertTrue(first.ready);self.assertEqual(bytes(self.memory[:512]),saved)
        self.assertEqual(self.lib.arm_arena_used(C.byref(self.arena)),4096)
        self.fail=None;rc,third=self.map();self.assertEqual(rc,0);self.assertEqual(third.base,first.base+4096)
    def test_failed_sync_releases_reservation(self):
        self.fail='sync';rc,image=self.map();self.assertEqual(rc,-72);self.assertFalse(image.ready)
        self.assertEqual(self.lib.arm_arena_used(C.byref(self.arena)),0)
    def test_missing_import_never_published(self):
        import struct
        from test_arm_link import external_fixture
        data=external_fixture();struct.pack_into('<I',data,76,5)
        rc,image=self.map(data);self.assertEqual(rc,-20);self.assertFalse(image.ready)
        self.assertEqual(self.events,[]);self.assertEqual(self.lib.arm_arena_used(C.byref(self.arena)),0)
    def test_exhaustion_preserves_existing_modules(self):
        for i in range(4):self.assertEqual(self.map()[0],0)
        saved=bytes(self.memory);rc,image=self.map();self.assertEqual(rc,-62)
        self.assertFalse(image.ready);self.assertEqual(bytes(self.memory),saved)
