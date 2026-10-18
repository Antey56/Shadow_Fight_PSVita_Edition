import ctypes as C
import pathlib
import sys
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from registration_host import load
DLL=next((p for p in (ROOT/'build-host/jni_objects_host.dll',ROOT/'build-host/jni_objects_host.so')
          if p.exists()),ROOT/'build-host/jni_objects_host.so')
P=C.c_void_p
class Value(C.Union):
    _fields_=[('l',P),('z',C.c_uint8),('i',C.c_int32),('j',C.c_int64),('d',C.c_double)]

@unittest.skipUnless(DLL.exists(),'build jni_objects_host first')
class StringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.lib=load(DLL)
    def setUp(self):
        self.env=self.lib.sf_jni_bridge_create(b'.',4096);self.assertTrue(self.env)
        self.table=C.cast(C.cast(self.env,C.POINTER(P))[0],C.POINTER(P))
    def tearDown(self):self.lib.sf_jni_bridge_destroy(self.env)
    def fn(self,slot,ret,*args):return C.CFUNCTYPE(ret,P,*args)(self.table[slot])
    def string(self,b):return self.fn(167,P,C.c_char_p)(self.env,b)
    def clear(self):self.fn(17,None)(self.env)
    def check(self):return self.fn(228,C.c_uint8)(self.env)
    def lengths(self,r):return (self.fn(168,C.c_int32,P)(self.env,r),self.fn(164,C.c_int32,P)(self.env,r))
    def chars(self,r):return self.fn(169,P,P,P)(self.env,r,None)
    def release(self,r,p):self.fn(170,None,P,P)(self.env,r,p)
    def static(self,name=b'java.lang.String',initialize=0,loader=None):
        cls=self.fn(6,P,C.c_char_p)(self.env,b'java/lang/Class')
        method=self.fn(113,P,P,C.c_char_p,C.c_char_p)(self.env,cls,b'forName',b'(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;')
        args=(Value*3)();args[0].l=self.string(name);args[1].z=initialize;args[2].l=loader
        return self.fn(116,P,P,P,C.POINTER(Value))(self.env,cls,method,args)
    def test_ascii_and_copy_flag(self):
        r=self.string(b'asset');self.assertEqual(self.lengths(r),(5,5))
        copied=C.c_uint8();p=self.fn(169,P,P,C.POINTER(C.c_uint8))(self.env,r,C.byref(copied))
        self.assertEqual(copied.value,1);self.assertEqual(C.string_at(p),b'asset');self.release(r,p)
    def test_empty(self):
        r=self.string(b'');self.assertTrue(r);self.assertEqual(self.lengths(r),(0,0))
        p=self.chars(r);self.assertTrue(p);self.assertEqual(C.string_at(p),b'');self.release(r,p)
    def test_encoded_nul(self):
        r=self.string(b'a\xc0\x80b');self.assertEqual(self.lengths(r),(4,3))
        p=self.chars(r);self.assertEqual(C.string_at(p),b'a\xc0\x80b');self.release(r,p)
    def test_non_ascii(self):
        text='тест'.encode();r=self.string(text);self.assertEqual(self.lengths(r),(8,4))
        p=self.chars(r);self.assertEqual(C.string_at(p),text);self.release(r,p)
    def test_surrogate_pair(self):
        r=self.string(b'\xed\xa0\xbd\xed\xb8\x80');self.assertEqual(self.lengths(r),(6,2))
    def test_malformed_encodings(self):
        for b in [b'\xc0\x81',b'\xc1\xbf',b'\xe0\x80\x80',b'\xe1\x80',b'\xf0\x9f\x98\x80',b'\x80']:
            with self.subTest(b=b):self.assertFalse(self.string(b));self.assertTrue(self.check());self.clear()
    def test_string_limit(self):
        self.assertFalse(self.string(b'x'*65537));self.assertTrue(self.check())
    def test_null_string(self):self.assertFalse(self.string(None));self.assertTrue(self.check())
    def test_global_copy_lifetime(self):
        r=self.string(b'owned');g=self.fn(21,P,P)(self.env,r)
        self.fn(23,None,P)(self.env,r)
        p=self.chars(g);self.assertEqual(C.string_at(p),b'owned');self.release(g,p)
        self.fn(22,None,P)(self.env,g)
    def test_wrong_owner_release(self):
        a=self.string(b'a');b=self.string(b'b');p=self.chars(a)
        self.release(b,p);self.assertTrue(self.check());self.clear();self.release(a,p);self.assertFalse(self.check())
    def test_double_release(self):
        r=self.string(b'a');p=self.chars(r);self.release(r,p);self.release(r,p);self.assertTrue(self.check())
    def test_release_during_pending_error(self):
        r=self.string(b'a');p=self.chars(r);self.string(b'\xff')
        self.release(r,p);self.assertTrue(self.check());self.clear()
        p=self.chars(r);self.assertTrue(p);self.release(r,p)
    def test_lease_capacity(self):
        r=self.string(b'x');pointers=[self.chars(r) for _ in range(16)]
        self.assertTrue(all(pointers));self.assertFalse(self.chars(r));self.assertTrue(self.check())
        for p in pointers:self.release(r,p)
        self.clear();p=self.chars(r);self.assertTrue(p);self.release(r,p)
    def test_class_for_name_known(self):self.assertTrue(self.static());self.assertFalse(self.check())
    def test_class_for_name_unknown(self):self.assertFalse(self.static(b'unknown.Class'));self.assertTrue(self.check())
    def test_class_initialization_rejected(self):self.assertFalse(self.static(initialize=1));self.assertTrue(self.check())
    def test_class_name_invalid(self):self.assertFalse(self.static(b'java/lang/String'));self.assertTrue(self.check())
    def test_string_class(self):
        r=self.string(b'x');self.assertTrue(self.fn(31,P,P)(self.env,r))
