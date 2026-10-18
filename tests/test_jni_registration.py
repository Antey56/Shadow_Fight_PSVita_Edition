import ctypes as C
import pathlib
import sys
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from registration_host import Declaration,CodeRange,Method,load,functions
DLL=next((p for p in (ROOT/'build-host/jni_objects_host.dll',ROOT/'build-host/jni_objects_host.so')
          if p.exists()),ROOT/'build-host/jni_objects_host.so')
CLASS=b'com/unity3d/player/UnityPlayer'

@unittest.skipUnless(DLL.exists(),'build jni_objects_host first')
class RegistrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.lib=load(DLL)
    def setUp(self):
        self.env=self.lib.sf_jni_bridge_create(b'.',4096);self.assertTrue(self.env)
        self.f=functions(self.env)
        self.decl=(Declaration*2)(Declaration(CLASS,b'nativeRender',b'()Z'),Declaration(CLASS,b'nativePause',b'()Z'))
        self.ranges=(CodeRange*1)(CodeRange(0x1000,0x1100))
        self.assertEqual(self.lib.sf_jni_bridge_configure_natives(self.env,self.decl,2,self.ranges,1),0)
        self.cls=self.f['find'](self.env,CLASS);self.assertTrue(self.cls)
    def tearDown(self):self.lib.sf_jni_bridge_destroy(self.env)
    def methods(self,ptr=0x1001):return (Method*2)(Method(b'nativeRender',b'()Z',ptr),Method(b'nativePause',b'()Z',0x1011))
    def lookup(self,name=b'nativeRender'):return self.lib.sf_jni_bridge_registered(self.env,CLASS,name,b'()Z')
    def test_register_and_lookup(self):
        self.assertEqual(self.f['register'](self.env,self.cls,self.methods(),2),0)
        self.assertEqual(self.lookup(),0x1001);self.assertEqual(self.lookup(b'nativePause'),0x1011)
    def test_idempotent(self):
        for _ in range(2):self.assertEqual(self.f['register'](self.env,self.cls,self.methods(),2),0)
    def test_address_conflict(self):
        self.f['register'](self.env,self.cls,self.methods(),2)
        self.assertLess(self.f['register'](self.env,self.cls,self.methods(0x1021),2),0)
        self.assertEqual(self.lookup(),0x1001);self.assertTrue(self.f['check'](self.env))
    def test_unknown_method_rolls_back(self):
        m=self.methods();m[1].name=b'unknown'
        self.assertLess(self.f['register'](self.env,self.cls,m,2),0);self.assertEqual(self.lookup(),0)
    def test_signature_mismatch_rolls_back(self):
        m=self.methods();m[1].signature=b'()V'
        self.assertLess(self.f['register'](self.env,self.cls,m,2),0);self.assertEqual(self.lookup(),0)
    def test_outside_range(self):
        self.assertLess(self.f['register'](self.env,self.cls,self.methods(0x1201),2),0)
        self.assertEqual(self.lookup(),0)
    def test_arm_alignment(self):
        self.assertLess(self.f['register'](self.env,self.cls,self.methods(0x1002),2),0)
    def test_thumb_end_boundary(self):
        self.assertLess(self.f['register'](self.env,self.cls,self.methods(0x1101),2),0)
    def test_arm_address(self):
        self.assertEqual(self.f['register'](self.env,self.cls,self.methods(0x1000),2),0)
    def test_invalid_counts(self):
        for count in [0,-1,129]:
            self.assertLess(self.f['register'](self.env,self.cls,self.methods(),count),0)
            self.f['clear'](self.env)
    def test_deleted_class(self):
        self.f['delete'](self.env,self.cls)
        self.assertLess(self.f['register'](self.env,self.cls,self.methods(),2),0)
    def test_records_own_strings(self):
        name=C.create_string_buffer(b'nativeRender');sig=C.create_string_buffer(b'()Z')
        m=(Method*1)(Method(C.cast(name,C.c_char_p),C.cast(sig,C.c_char_p),0x1001))
        self.assertEqual(self.f['register'](self.env,self.cls,m,1),0)
        name.value=b'xxxx';sig.value=b'()V'
        self.assertEqual(self.lookup(),0x1001)
    def test_reconfigure_rejected(self):
        self.assertLess(self.lib.sf_jni_bridge_configure_natives(self.env,self.decl,2,self.ranges,1),0)
