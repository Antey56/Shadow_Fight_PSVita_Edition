import ctypes as C
import os
import pathlib
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from asset_host import load_backend
DLL=next((p for p in (ROOT/'build-host/jni_objects_host.dll',ROOT/'build-host/jni_objects_host.so')
          if p.exists()),ROOT/'build-host/jni_objects_host.so')
P=C.c_void_p


@unittest.skipUnless(DLL.exists(),'build jni_objects_host first')
class BridgeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib=load_backend(DLL)
        for name,args,ret in [
            ('sf_jni_bridge_create',[C.c_char_p,C.c_size_t],P),
            ('sf_jni_bridge_destroy',[P],None),('sf_jni_bridge_context',[P],P),
            ('sf_jni_bridge_assets',[P,P],P),
            ('sf_jni_bridge_require',[C.POINTER(C.c_uint),C.c_size_t],C.c_int)]:
            f=getattr(cls.lib,name);f.argtypes=args;f.restype=ret

    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(dir=ROOT/'tests')
        (pathlib.Path(self.temp.name)/'data').write_bytes(b'through-jni-table')
        self.env=self.lib.sf_jni_bridge_create(os.fsencode(self.temp.name),4096)
        self.assertTrue(self.env)
        self.table=C.cast(C.cast(self.env,C.POINTER(P))[0],C.POINTER(P))
        self.context=self.lib.sf_jni_bridge_context(self.env)

    def tearDown(self):
        self.lib.sf_jni_bridge_destroy(self.env);self.temp.cleanup()

    def fn(self,slot,ret,*args):
        return C.CFUNCTYPE(ret,P,*args)(self.table[slot])

    def method(self):
        cls=self.fn(6,P,C.c_char_p)(self.env,b'android/content/Context')
        return self.fn(33,P,P,C.c_char_p,C.c_char_p)(self.env,cls,b'getAssets',b'()Landroid/content/res/AssetManager;')

    def test_read_via_table(self):
        obj=self.fn(36,P,P,P,P)(self.env,self.context,self.method(),None)
        manager=self.lib.sf_jni_bridge_assets(self.env,obj);self.assertTrue(manager)
        a=self.lib.AAssetManager_open(manager,b'data',2);self.assertTrue(a)
        try:
            buf=C.create_string_buffer(64);n=self.lib.AAsset_read(a,buf,64)
            self.assertEqual(buf.raw[:n],b'through-jni-table')
        finally:self.lib.AAsset_close(a)

    def test_slots_and_gate(self):
        slots=(4,6,17,21,22,23,25,31,33,34,35,36,219,228)
        self.assertEqual({i for i in range(233) if self.table[i]},set(slots))
        self.assertEqual(self.lib.sf_jni_bridge_require((C.c_uint*len(slots))(*slots),len(slots)),0)
        self.assertLess(self.lib.sf_jni_bridge_require((C.c_uint*2)(36,113),2),0)

    def test_unknown_class_and_clear(self):
        self.assertFalse(self.fn(6,P,C.c_char_p)(self.env,b'java/lang/String'))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))
        self.assertFalse(self.fn(6,P,C.c_char_p)(self.env,b'android/content/Context'))
        self.fn(17,None)(self.env)
        self.assertTrue(self.fn(6,P,C.c_char_p)(self.env,b'android/content/Context'))

    def test_bad_method_signature(self):
        cls=self.fn(31,P,P)(self.env,self.context)
        self.assertFalse(self.fn(33,P,P,C.c_char_p,C.c_char_p)(self.env,cls,b'getAssets',b'()V'))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_global_ref_and_stale(self):
        m=self.method();g=self.fn(21,P,P)(self.env,self.context)
        self.fn(23,None,P)(self.env,self.context)
        self.assertTrue(self.fn(36,P,P,P,P)(self.env,g,m,None))
        self.fn(22,None,P)(self.env,g)
        self.assertFalse(self.fn(36,P,P,P,P)(self.env,g,m,None))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_wrong_receiver(self):
        m=self.method();asset=self.fn(36,P,P,P,P)(self.env,self.context,m,None)
        self.assertFalse(self.fn(36,P,P,P,P)(self.env,asset,m,None))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_forged_method(self):
        self.assertFalse(self.fn(36,P,P,P,P)(self.env,self.context,12345,None))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_local_ref_copy(self):
        m=self.method();copy=self.fn(25,P,P)(self.env,self.context)
        self.fn(23,None,P)(self.env,self.context)
        self.assertTrue(self.fn(36,P,P,P,P)(self.env,copy,m,None))
