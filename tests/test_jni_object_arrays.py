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
class ObjectArrayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib=load(DLL)
        cls.lib.sf_jni_bridge_context.argtypes=[P];cls.lib.sf_jni_bridge_context.restype=P
    def setUp(self):
        self.env=self.lib.sf_jni_bridge_create(b'.',4096);self.assertTrue(self.env)
        self.table=C.cast(C.cast(self.env,C.POINTER(P))[0],C.POINTER(P))
    def tearDown(self):self.lib.sf_jni_bridge_destroy(self.env)
    def fn(self,slot,ret,*args):return C.CFUNCTYPE(ret,P,*args)(self.table[slot])
    def cls(self,name=b'java/lang/String'):return self.fn(6,P,C.c_char_p)(self.env,name)
    def string(self,text=b'value'):return self.fn(167,P,C.c_char_p)(self.env,text)
    def new(self,n=2,cls=None,value=None):
        return self.fn(172,P,I,P,P)(self.env,n,self.cls() if cls is None else cls,value)
    def get(self,a,i=0):return self.fn(173,P,P,I)(self.env,a,i)
    def set(self,a,i=0,value=None):self.fn(174,None,P,I,P)(self.env,a,i,value)
    def length(self,a):return self.fn(171,I,P)(self.env,a)
    def delete(self,a):self.fn(23,None,P)(self.env,a)
    def check(self):return self.fn(228,C.c_uint8)(self.env)
    def clear(self):self.fn(17,None)(self.env)
    def text(self,s):
        p=self.fn(169,P,P,P)(self.env,s,None);self.assertTrue(p)
        value=C.string_at(p);self.fn(170,None,P,P)(self.env,s,p);return value

    def test_null_initial_and_length(self):
        a=self.new(3);self.assertEqual(self.length(a),3)
        self.assertIsNone(self.get(a,2));self.assertFalse(self.check())
    def test_string_retained_after_original_deleted(self):
        s=self.string();a=self.new(value=s);self.delete(s)
        x=self.get(a);self.assertNotEqual(x,s);self.assertEqual(self.text(x),b'value')
        y=self.get(a,1);self.assertNotEqual(x,y);self.assertEqual(self.text(y),b'value')
    def test_returned_local_survives_array_delete(self):
        s=self.string();a=self.new(value=s);x=self.get(a);self.delete(s);self.delete(a)
        self.assertEqual(self.text(x),b'value')
    def test_replace_null_and_shared_global_array(self):
        a=self.new();g=self.fn(21,P,P)(self.env,a);s=self.string(b'replaced')
        self.set(a,1,s);self.delete(s);self.delete(a)
        x=self.get(g,1);self.assertEqual(self.text(x),b'replaced')
        self.set(g,1);self.assertIsNone(self.get(g,1));self.assertEqual(self.text(x),b'replaced')
        self.fn(22,None,P)(self.env,g);self.assertFalse(self.check())
    def test_global_retains_children_across_local_clear(self):
        s=self.string();a=self.new(value=s);g=self.fn(21,P,P)(self.env,a)
        self.lib.sf_jni_bridge_clear_locals.argtypes=[P]
        self.lib.sf_jni_bridge_clear_locals(self.env)
        self.assertEqual(self.text(self.get(g)),b'value')
        self.fn(22,None,P)(self.env,g)
    def test_incompatible_store_is_transactional(self):
        a=self.new(value=self.string());context=self.lib.sf_jni_bridge_context(self.env)
        self.set(a,0,context);self.assertTrue(self.check());self.clear()
        self.assertEqual(self.text(self.get(a)),b'value')
    def test_wrong_initial_type_rejected(self):
        self.assertIsNone(self.new(value=self.cls()));self.assertTrue(self.check())
    def test_bounds_and_pending_exception(self):
        a=self.new(value=self.string())
        for index in (-1,2,2**31-1):
            self.assertIsNone(self.get(a,index));self.assertTrue(self.check())
            self.set(a,0,None);self.clear();self.assertEqual(self.text(self.get(a)),b'value')
            self.set(a,index,None);self.assertTrue(self.check());self.clear()
    def test_negative_and_large_lengths(self):
        for n in (-1,4097,2**31-1):
            self.assertIsNone(self.new(n));self.assertTrue(self.check());self.clear()
        a=self.new(0);self.assertEqual(self.length(a),0)
        self.assertIsNone(self.get(a));self.assertTrue(self.check())
    def test_nonarray_and_nonclass_rejected(self):
        s=self.string();self.assertIsNone(self.new(cls=s));self.assertTrue(self.check());self.clear()
        self.assertIsNone(self.get(s));self.assertTrue(self.check());self.clear()
        self.set(s);self.assertTrue(self.check());self.clear()
        self.assertEqual(self.length(s),0);self.assertTrue(self.check())
    def test_object_array_accepts_class_and_primitive_array(self):
        a=self.new(cls=self.cls(b'java/lang/Object'));s=self.cls()
        self.set(a,0,s);self.assertTrue(self.get(a));self.assertFalse(self.check())
        primitive=self.fn(179,P,I)(self.env,2);self.set(a,1,primitive);self.delete(primitive)
        self.assertEqual(self.length(self.get(a,1)),2);self.assertFalse(self.check())
    def test_nested_and_self_arrays_rejected(self):
        a=self.new(cls=self.cls(b'java/lang/Object'))
        self.set(a,0,a);self.assertTrue(self.check());self.clear()
        self.assertIsNone(self.get(a));self.assertFalse(self.check())
    def test_stale_value_rejected_without_mutation(self):
        a=self.new();s=self.string();self.delete(s);self.set(a,0,s)
        self.assertTrue(self.check());self.clear();self.assertIsNone(self.get(a))
    def test_elements_do_not_consume_public_reference_slots(self):
        s=self.string();a=self.new(4096,value=s);self.delete(s)
        for _ in range(200):
            x=self.get(a,4095);self.assertEqual(self.text(x),b'value');self.delete(x)
        self.assertFalse(self.check())
    def test_repeated_allocation_releases_storage(self):
        cls=self.cls()
        for _ in range(100):
            a=self.new(4096,cls);self.assertTrue(a);self.delete(a)
        self.assertFalse(self.check())
    def test_reference_capacity_failure_preserves_element(self):
        a=self.new(value=self.string());held=[]
        while True:
            x=self.get(a)
            if not x:break
            held.append(x)
        self.assertTrue(self.check());self.clear()
        self.delete(held.pop());self.assertEqual(self.text(self.get(a)),b'value')
    def test_direct_buffer_assignable_to_bytebuffer(self):
        storage=C.create_string_buffer(b'abc')
        b=self.fn(229,P,P,C.c_int64)(self.env,C.cast(storage,P),3)
        a=self.new(cls=self.cls(b'java/nio/ByteBuffer'),value=b);self.assertTrue(a)
        self.delete(b);x=self.get(a);self.assertEqual(self.fn(231,C.c_int64,P)(self.env,x),3)

    def test_aggregate_budget_released_after_failure(self):
        cls=self.cls();arrays=[]
        while True:
            a=self.new(4096,cls)
            if not a:break
            arrays.append(a)
        self.assertTrue(self.check());self.clear()
        for a in arrays:self.delete(a)
        self.assertTrue(self.new(4096,cls));self.assertFalse(self.check())
    def test_primitive_component_type_checked(self):
        byte=self.fn(176,P,I)(self.env,2)
        cls=self.cls(b'[B');a=self.new(cls=cls,value=byte);self.assertTrue(a)
        ints=self.fn(179,P,I)(self.env,2);self.set(a,0,ints)
        self.assertTrue(self.check());self.clear()
        self.assertEqual(self.length(self.get(a)),2)
