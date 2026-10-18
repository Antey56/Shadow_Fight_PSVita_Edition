import ctypes as C
import pathlib
import sys
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from registration_host import load
DLL=next((p for p in (ROOT/'build-host/jni_objects_host.dll',ROOT/'build-host/jni_objects_host.so')
          if p.exists()),ROOT/'build-host/jni_objects_host.so')
P=C.c_void_p;I=C.c_int32

@unittest.skipUnless(DLL.exists(),'build jni_objects_host first')
class ArrayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.lib=load(DLL)
    def setUp(self):
        self.env=self.lib.sf_jni_bridge_create(b'.',4096);self.assertTrue(self.env)
        self.table=C.cast(C.cast(self.env,C.POINTER(P))[0],C.POINTER(P))
    def tearDown(self):self.lib.sf_jni_bridge_destroy(self.env)
    def fn(self,slot,ret,*args):return C.CFUNCTYPE(ret,P,*args)(self.table[slot])
    def new(self,kind=1,n=4):return self.fn(175+kind,P,I)(self.env,n)
    def check(self):return self.fn(228,C.c_uint8)(self.env)
    def clear(self):self.fn(17,None)(self.env)
    def set(self,a,values,kind=1,start=0):self.fn(207+kind,None,P,I,I,P)(self.env,a,start,len(values),values)
    def read(self,a,n,kind=1,dtype=C.c_int8,start=0):
        data=(dtype*n)();self.fn(199+kind,None,P,I,I,P)(self.env,a,start,n,data);return list(data)
    def elements(self,a,kind=1):return self.fn(183+kind,P,P,P)(self.env,a,None)
    def release(self,a,p,mode=0,kind=1):self.fn(191+kind,None,P,P,I)(self.env,a,p,mode)
    def delete(self,a):self.fn(23,None,P)(self.env,a)

    def test_all_primitive_types(self):
        cases=[(C.c_uint8,[0,1,0]),(C.c_int8,[-1,2,-3]),(C.c_uint16,[0,0x20ac,0xffff]),
               (C.c_int16,[-300,0,300]),(I,[-10000000,0,10000000]),
               (C.c_int64,[-2**40,0,2**40]),(C.c_float,[-1.25,0,1.25]),(C.c_double,[-1e100,0,1e100])]
        for kind,(dtype,values) in enumerate(cases):
            with self.subTest(kind=kind):
                a=self.new(kind);self.assertTrue(a)
                self.assertEqual(self.fn(171,I,P)(self.env,a),4)
                self.assertEqual(self.read(a,4,kind,dtype),[0]*4)
                self.set(a,(dtype*3)(*values),kind,1)
                self.assertEqual(self.read(a,4,kind,dtype),[0]+values)
                copied=C.c_uint8();p=self.fn(183+kind,P,P,C.POINTER(C.c_uint8))(self.env,a,C.byref(copied))
                self.assertTrue(p);self.assertEqual(copied.value,1)
                C.cast(p,C.POINTER(dtype))[0]=1
                self.release(a,p,0,kind)
                self.assertEqual(self.read(a,4,kind,dtype),[1]+values);self.delete(a)
        self.assertFalse(self.check())

    def test_abort_discards_copy(self):
        a=self.new();p=self.elements(a);C.cast(p,C.POINTER(C.c_int8))[0]=42
        self.release(a,p,2);self.assertEqual(self.read(a,4),[0]*4)
    def test_commit_retains_copy(self):
        a=self.new();p=self.elements(a);v=C.cast(p,C.POINTER(C.c_int8));v[0]=7
        self.release(a,p,1);self.assertEqual(self.read(a,4),[7,0,0,0])
        v[0]=9;self.release(a,p,0);self.assertEqual(self.read(a,4),[9,0,0,0])
    def test_empty_array(self):
        a=self.new(n=0);self.assertTrue(a);self.assertEqual(self.fn(171,I,P)(self.env,a),0)
        p=self.elements(a);self.assertTrue(p);self.release(a,p)
    def test_negative_size(self):self.assertFalse(self.new(n=-1));self.assertTrue(self.check())
    def test_per_array_limit(self):self.assertFalse(self.new(n=1024**2+1));self.assertTrue(self.check())
    def test_aggregate_limit_recovers(self):
        arrays=[self.new(n=1024**2) for _ in range(8)];self.assertTrue(all(arrays))
        self.assertFalse(self.new(n=1));self.assertTrue(self.check())
        self.delete(arrays[0]);self.clear();self.assertTrue(self.new(n=1024**2))
    def test_regions_and_overflow(self):
        a=self.new();data=(C.c_int8*4)(1,2,3,4);self.set(a,data)
        for start,count in [(-1,1),(0,-1),(4,1),(5,0),(2**31-1,2**31-1)]:
            self.fn(208,None,P,I,I,P)(self.env,a,start,count,data)
            self.assertTrue(self.check());self.clear();self.assertEqual(self.read(a,4),[1,2,3,4])
    def test_zero_region_at_end(self):
        a=self.new();self.fn(200,None,P,I,I,P)(self.env,a,4,0,None);self.assertFalse(self.check())
    def test_wrong_element_type(self):
        a=self.new(kind=4);self.assertFalse(self.elements(a,1));self.assertTrue(self.check())
    def test_global_lifetime(self):
        a=self.new();g=self.fn(21,P,P)(self.env,a);self.delete(a)
        self.set(g,(C.c_int8*4)(4,3,2,1));self.assertEqual(self.read(g,4),[4,3,2,1])
    def test_wrong_lease_owner(self):
        a=self.new();b=self.new();p=self.elements(a);self.release(b,p)
        self.assertTrue(self.check());self.clear();self.release(a,p);self.assertFalse(self.check())
    def test_bad_release_mode_preserves_lease(self):
        a=self.new();p=self.elements(a);self.release(a,p,3);self.assertTrue(self.check())
        self.clear();self.release(a,p,2);self.assertFalse(self.check())
    def test_release_during_error(self):
        a=self.new();p=self.elements(a);self.new(n=-1);self.release(a,p,2);self.assertTrue(self.check())
        self.clear();self.assertTrue(self.elements(a))
    def test_array_class(self):
        a=self.new();self.assertTrue(self.fn(31,P,P)(self.env,a))
        self.assertTrue(self.fn(6,P,C.c_char_p)(self.env,b'[B'))
    def test_copied_lease_count_limit(self):
        a=self.new();p=[self.elements(a) for _ in range(16)];self.assertTrue(all(p))
        self.assertFalse(self.elements(a));self.assertTrue(self.check())
        for v in p:self.release(a,v,2)
        self.clear();self.assertTrue(self.elements(a))
    def test_lease_memory_limit(self):
        a=self.new(n=1024**2);p=[self.elements(a) for _ in range(4)];self.assertTrue(all(p))
        self.assertFalse(self.elements(a));self.assertTrue(self.check())
        for v in p:self.release(a,v,2)
    def test_direct_buffer_alias(self):
        memory=C.create_string_buffer(b'PCM!')
        a=self.fn(229,P,P,C.c_int64)(self.env,C.cast(memory,P),4);self.assertTrue(a)
        address=self.fn(230,P,P)(self.env,a);self.assertEqual(address,C.addressof(memory))
        self.assertEqual(self.fn(231,C.c_int64,P)(self.env,a),4)
        C.cast(address,C.POINTER(C.c_char))[0]=b'X';self.assertEqual(memory.raw[:4],b'XCM!')
        self.delete(a);self.assertEqual(memory.raw[:4],b'XCM!')
    def test_direct_buffer_global_alias(self):
        memory=C.create_string_buffer(8);a=self.fn(229,P,P,C.c_int64)(self.env,C.cast(memory,P),8)
        g=self.fn(21,P,P)(self.env,a);self.delete(a)
        self.assertEqual(self.fn(230,P,P)(self.env,g),C.addressof(memory))
    def test_direct_buffer_invalid_capacity(self):
        memory=C.create_string_buffer(1)
        for n in [-1,2**31]:
            self.assertFalse(self.fn(229,P,P,C.c_int64)(self.env,C.cast(memory,P),n));self.assertTrue(self.check());self.clear()
    def test_direct_buffer_null(self):
        self.assertFalse(self.fn(229,P,P,C.c_int64)(self.env,None,0));self.assertTrue(self.check())
    def test_non_direct_object(self):
        a=self.new();self.assertFalse(self.fn(230,P,P)(self.env,a))
        self.assertEqual(self.fn(231,C.c_int64,P)(self.env,a),-1);self.assertFalse(self.check())
