import ctypes as C
import os
import pathlib
import sys
import tempfile
import subprocess
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
        slots=(4,6,13,15,17,18,21,22,23,25,31,33,34,35,36,113,114,115,116,164,167,168,169,170,171,172,173,174,*range(175,215),215,219,228,229,230,231)
        self.assertEqual({i for i in range(233) if self.table[i]},set(slots))
        self.assertEqual(self.lib.sf_jni_bridge_require((C.c_uint*len(slots))(*slots),len(slots)),0)
        self.assertLess(self.lib.sf_jni_bridge_require((C.c_uint*2)(36,94),2),0)

    def test_unknown_class_and_clear(self):
        self.assertFalse(self.fn(6,P,C.c_char_p)(self.env,b'java/lang/Unsupported'))
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

    def pending_object(self):
        self.assertFalse(self.fn(6,P,C.c_char_p)(self.env,b'unknown/Class'))
        error=self.fn(15,P)(self.env);self.assertTrue(error)
        return error

    def test_exception_round_trip(self):
        error=self.pending_object()
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))
        self.fn(17,None)(self.env)
        self.assertEqual(self.fn(13,C.c_int32,P)(self.env,error),0)
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))
        self.assertTrue(self.fn(15,P)(self.env))

    def test_exception_absent(self):
        self.assertFalse(self.fn(15,P)(self.env))
        self.assertFalse(self.fn(228,C.c_uint8)(self.env))

    def test_throw_wrong_type(self):
        self.assertLess(self.fn(13,C.c_int32,P)(self.env,self.context),0)
        self.assertTrue(self.fn(15,P)(self.env))

    def test_throw_null(self):
        self.assertLess(self.fn(13,C.c_int32,P)(self.env,None),0)
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_throw_deleted_error(self):
        error=self.pending_object();self.fn(17,None)(self.env)
        self.fn(23,None,P)(self.env,error)
        self.assertLess(self.fn(13,C.c_int32,P)(self.env,error),0)
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_global_error_survives_clear_and_local_delete(self):
        error=self.pending_object();self.fn(17,None)(self.env)
        glob=self.fn(21,P,P)(self.env,error);self.assertTrue(glob)
        self.fn(23,None,P)(self.env,error)
        self.assertEqual(self.fn(13,C.c_int32,P)(self.env,glob),0)
        self.fn(22,None,P)(self.env,glob)
        self.assertTrue(self.fn(15,P)(self.env))

    def test_pending_error_preserved_on_throw(self):
        error=self.pending_object()
        self.assertLess(self.fn(13,C.c_int32,P)(self.env,self.context),0)
        self.fn(17,None)(self.env)
        self.assertEqual(self.fn(13,C.c_int32,P)(self.env,error),0)

    def test_exception_reference_capacity(self):
        for _ in range(63):self.assertTrue(self.lib.sf_jni_bridge_context(self.env))
        self.fn(6,P,C.c_char_p)(self.env,b'unknown/Class')
        self.assertFalse(self.fn(15,P)(self.env))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))
        self.fn(23,None,P)(self.env,self.context)
        self.assertTrue(self.fn(15,P)(self.env))

    def test_class_of_class(self):
        cls=self.fn(31,P,P)(self.env,self.context)
        meta=self.fn(31,P,P)(self.env,cls);self.assertTrue(meta)
        self.assertTrue(self.fn(31,P,P)(self.env,meta))
        self.assertFalse(self.fn(228,C.c_uint8)(self.env))

    def test_class_get_class_loader(self):
        cls=self.fn(31,P,P)(self.env,self.context)
        meta=self.fn(31,P,P)(self.env,cls)
        method=self.fn(33,P,P,C.c_char_p,C.c_char_p)(self.env,meta,b'getClassLoader',b'()Ljava/lang/ClassLoader;')
        self.assertTrue(method)
        loader=self.fn(36,P,P,P,P)(self.env,cls,method,None);self.assertTrue(loader)
        self.assertTrue(self.fn(31,P,P)(self.env,loader))

    def test_get_class_loader_wrong_receiver(self):
        meta=self.fn(6,P,C.c_char_p)(self.env,b'java/lang/Class')
        method=self.fn(33,P,P,C.c_char_p,C.c_char_p)(self.env,meta,b'getClassLoader',b'()Ljava/lang/ClassLoader;')
        self.assertFalse(self.fn(36,P,P,P,P)(self.env,self.context,method,None))
        self.assertTrue(self.fn(228,C.c_uint8)(self.env))

    def test_class_loader_class_available(self):
        cls=self.fn(6,P,C.c_char_p)(self.env,b'java/lang/ClassLoader');self.assertTrue(cls)
        self.assertTrue(self.fn(31,P,P)(self.env,cls))

    def test_fatal_error_terminates_child(self):
        script = r"""
import ctypes as C, sys
if sys.platform == 'win32':
    C.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
else:
    import resource
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
lib=C.CDLL(sys.argv[1]); lib.sf_jni_bridge_create.argtypes=[C.c_char_p,C.c_size_t]
lib.sf_jni_bridge_create.restype=C.c_void_p
e=lib.sf_jni_bridge_create(b'.',4096)
if not e: sys.exit(99)
t=C.cast(C.cast(e,C.POINTER(C.c_void_p))[0],C.POINTER(C.c_void_p))
C.CFUNCTYPE(None,C.c_void_p,C.c_char_p)(t[18])(e,None if sys.argv[2]=='null' else b'bootstrap %s failure')
print('UNREACHABLE',flush=True)
sys.exit(0)
"""
        for case, expected in [('message',b'bootstrap %s failure'),('null',b'(null)')]:
            with self.subTest(case=case):
                child=subprocess.run([sys.executable,'-c',script,str(DLL.resolve()),case],
                                     capture_output=True,timeout=15)
                self.assertNotIn(child.returncode,(0,99))
                self.assertIn(b'JNI_FATAL: '+expected,child.stderr)
                self.assertNotIn(b'UNREACHABLE',child.stdout)
