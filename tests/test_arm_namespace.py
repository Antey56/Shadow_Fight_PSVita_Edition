import ctypes as C,pathlib,struct,unittest
from test_arm_exec import Image
from test_arm_symbols import symbols,Export
ROOT=pathlib.Path(__file__).resolve().parents[1]
DLL=next((p for p in (ROOT/'build-host/arm_namespace_host.dll',ROOT/'build-host/arm_namespace_host.so') if p.exists()),ROOT/'build-host/arm_namespace_host.so')
P=C.c_void_p;U=C.c_uint32;Z=C.c_size_t
class Module(C.Structure):
    _fields_=[('name',C.c_char*64),('file',P),('file_size',Z),('image',C.POINTER(Image)),
              ('generation',U),('references',U),('pins',U),('dependencies',U*16),
              ('dependency_count',C.c_uint),('state',C.c_uint),('order',C.c_uint64)]
class Namespace(C.Structure):
    _fields_=[('modules',Module*16),('serial',C.c_uint64),('error',C.c_char*128),('error_pending',C.c_uint)]
@unittest.skipUnless(DLL.exists(),'build arm_namespace_host first')
class NamespaceTests(unittest.TestCase):
    def setUp(self):
        self.lib=C.CDLL(str(DLL.resolve()));self.ns=Namespace();self.keep=[]
        self.lib.arm_namespace_init.argtypes=[C.POINTER(Namespace)]
        self.lib.arm_namespace_add.argtypes=[C.POINTER(Namespace),C.c_char_p,P,Z,C.POINTER(Image),C.POINTER(U),Z,C.POINTER(U)]
        for name in ['remove','begin_init','begin_fini','close','state']:
            getattr(self.lib,'arm_namespace_'+name).argtypes=[C.POINTER(Namespace),U]
        for name in ['end_init','end_fini']:
            getattr(self.lib,'arm_namespace_'+name).argtypes=[C.POINTER(Namespace),U,C.c_int]
        self.lib.arm_namespace_open.argtypes=[C.POINTER(Namespace),C.c_char_p,C.POINTER(U)]
        self.lib.arm_namespace_symbol.argtypes=[C.POINTER(Namespace),U,C.c_char_p,C.POINTER(Export)]
        self.lib.arm_namespace_error.argtypes=[C.POINTER(Namespace)];self.lib.arm_namespace_error.restype=C.c_char_p
        self.lib.arm_namespace_resolve.argtypes=[C.c_char_p,C.c_uint,C.c_uint,C.POINTER(U),P]
        self.lib.arm_namespace_init(C.byref(self.ns))
    def call(self,name,*args):return getattr(self.lib,'arm_namespace_'+name)(C.byref(self.ns),*args)
    def add(self,name=b'liba.so',deps=(),data=None,base=None):
        data=symbols() if data is None else data;buf=C.create_string_buffer(bytes(data));image=Image()
        image.ready=1;image.base=0x90000000+len(self.keep)*0x10000 if base is None else base
        image.bias=image.base;image.size=len(data);self.keep.append((buf,image))
        array=(U*len(deps))(*deps) if deps else None;handle=U()
        rc=self.call('add',name,buf,len(data),C.byref(image),array,len(deps),C.byref(handle));return rc,handle.value
    def ready(self,handle):
        self.assertEqual(self.call('begin_init',handle),0);self.assertEqual(self.call('end_init',handle,1),0)
    def open(self,name=b'liba.so'):
        handle=U();rc=self.call('open',name,C.byref(handle));return rc,handle.value
    def resolve(self,name=b'foo',typ=2):
        value=U(0x12345678);rc=self.lib.arm_namespace_resolve(name,1,typ,C.byref(value),C.byref(self.ns));return rc,value.value
    def test_ready_open_symbol_and_explicit_teardown(self):
        rc,h=self.add();self.assertEqual(rc,0)
        self.assertEqual(self.open()[0],-93);self.ready(h)
        self.assertEqual(self.open(),(0,h));self.assertEqual(self.open(),(0,h))
        value=Export();self.assertEqual(self.call('symbol',h,b'foo',C.byref(value)),1)
        self.assertEqual(value.value,0x90000191)
        self.assertEqual(self.call('begin_fini',h),-92)
        self.assertEqual(self.call('close',h),0);self.assertEqual(self.call('close',h),0)
        self.assertEqual(self.call('close',h),-93);self.assertEqual(self.call('remove',h),-93)
        self.assertEqual(self.call('begin_fini',h),0);self.assertEqual(self.open()[0],-93)
        self.assertEqual(self.call('end_fini',h,1),0);self.assertEqual(self.call('begin_init',h),-93)
        self.assertEqual(self.call('remove',h),0);self.assertEqual(self.call('state',h),0)
    def test_dependencies_pin_and_order_initialization(self):
        _,parent=self.add(b'libparent.so');_,child=self.add(b'libchild.so',(parent,))
        self.assertEqual(self.call('begin_init',child),-95);self.ready(parent);self.ready(child)
        self.assertEqual(self.call('begin_fini',parent),-92)
        self.assertEqual(self.call('begin_fini',child),0);self.assertEqual(self.call('end_fini',child,1),0)
        self.assertEqual(self.call('remove',child),0);self.assertEqual(self.call('begin_fini',parent),0)
    def test_failed_dependency_blocks_child_and_lookup(self):
        _,a=self.add();_,b=self.add(b'libb.so',(a,))
        self.assertEqual(self.call('begin_init',a),0);self.assertEqual(self.call('end_init',a,0),0)
        self.assertEqual(self.call('begin_init',b),-95);self.assertEqual(self.open()[0],-93)
        self.assertEqual(self.call('remove',a),-92);self.assertEqual(self.call('remove',b),0)
        self.assertEqual(self.call('remove',a),0)
    def test_stale_handle_does_not_affect_reused_slot(self):
        _,old=self.add();self.assertEqual(self.call('remove',old),0)
        _,new=self.add();self.assertNotEqual(old,new);self.ready(new);self.assertEqual(self.open(),(0,new))
        self.assertEqual(self.call('close',old),-91);self.assertEqual(self.ns.modules[0].references,1)
        self.assertEqual(self.call('begin_fini',new),-92)
    def test_error_consumed_once(self):
        self.assertEqual(self.open()[0],-91);self.assertIn(b'not registered',self.call('error'))
        self.assertIsNone(self.call('error'))
    def test_invalid_name_duplicate_dependencies_and_missing_parent(self):
        for name in (b'',b'../liba.so',b'/data/liba.so',b'x'*64):self.assertEqual(self.add(name)[0],-90)
        _,h=self.add();self.assertEqual(self.add()[0],-92)
        self.assertEqual(self.add(b'libb.so',(h,h))[0],-90)
        self.assertEqual(self.ns.modules[(h&255)-1].pins,0)
        self.assertEqual(self.add(b'libb.so',(0x1010,))[0],-95)
    def test_registration_order_survives_slot_reuse(self):
        _,a=self.add();_,b=self.add(b'libb.so');self.assertEqual(self.resolve(),(1,0x90000191))
        self.assertEqual(self.call('remove',a),0);self.add(b'libc.so')
        self.assertEqual(self.resolve(),(1,0x90010191))
        self.assertEqual(self.call('begin_init',b),0);self.assertEqual(self.resolve()[1],0x90010191)
    def test_lookup_errors_do_not_select_later_provider(self):
        data=symbols();data[300]=0x11 # object foo, rather than function
        self.add(data=data);self.add(b'libb.so')
        self.assertEqual(self.resolve(),(-98,0x12345678))
        self.assertEqual(self.resolve(b'absent'),(0,0x12345678))
    def test_symbol_requires_open_handle_and_clears_output_on_failure(self):
        _,h=self.add();self.ready(h);out=Export();out.value=123
        self.assertEqual(self.call('symbol',h,b'foo',C.byref(out)),-93);self.assertEqual(out.value,0)
        self.open();self.assertEqual(self.call('symbol',h,b'absent',C.byref(out)),-91)
    def test_capacity_and_reference_overflow(self):
        for i in range(16):self.assertEqual(self.add(('lib%d.so'%i).encode())[0],0)
        self.assertEqual(self.add(b'overflow.so')[0],-94)
        h=(self.ns.modules[0].generation<<8)|1;self.ready(h)
        self.ns.modules[0].references=0xffffffff
        self.assertEqual(self.open(b'lib0.so')[0],-96)
        self.ns.modules[0].pins=0xffffffff
        self.assertEqual(self.call('begin_fini',h),-92)
    def test_mapping_overlap_and_corruption(self):
        self.add();self.assertEqual(self.add(b'other.so',base=0x90000000)[0],-92)
        data=symbols();data[0]=0
        self.assertEqual(self.add(b'broken.so',data=data)[0],-97)
    def test_generation_retirement_and_invalid_transitions(self):
        self.ns.modules[0].generation=0xffffff
        _,h=self.add();self.assertEqual(h&255,2)
        self.assertEqual(self.call('end_init',h,1),-93)
        self.assertEqual(self.call('end_fini',h,1),-93)
        self.assertEqual(self.call('begin_init',h),0)
        self.assertEqual(self.call('remove',h),-93)
        self.assertEqual(self.call('end_init',h,1),0)
        self.assertEqual(self.call('begin_fini',h),0)
        self.assertEqual(self.call('end_fini',h,0),0)
        self.assertEqual(self.call('remove',h),0)

    def test_symbol_searches_breadth_first_dependency_closure(self):
        _,root=self.add(b'libroot.so')
        # Both middle and leaf have foo; breadth-first must select middle.
        _,leaf=self.add(b'libleaf.so',(root,))
        middle_data=symbols();middle_data[257:260]=b'bar'
        _,middle=self.add(b'libmiddle.so',(leaf,),data=middle_data)
        top_data=symbols();top_data[257:260]=b'bar'
        _,top=self.add(b'libtop.so',(middle,root),data=top_data)
        for h in (root,leaf,middle,top):self.ready(h)
        opened=U();self.assertEqual(self.call('open',b'libtop.so',C.byref(opened)),0)
        out=Export();self.assertEqual(self.call('symbol',opened,b'foo',C.byref(out)),1)
        self.assertEqual(out.value,0x90000191) # direct root before transitive leaf

    def test_duplicate_transitive_dependency_is_visited_once(self):
        _,root=self.add(b'libroot.so')
        data=symbols();data[257:260]=b'bar'
        _,left=self.add(b'libleft.so',(root,),data=data)
        _,right=self.add(b'libright.so',(root,),data=data)
        _,top=self.add(b'libtop.so',(left,right),data=data)
        for h in (root,left,right,top):self.ready(h)
        opened=U();self.call('open',b'libtop.so',C.byref(opened));out=Export()
        self.assertEqual(self.call('symbol',top,b'foo',C.byref(out)),1)
        self.assertEqual(out.value,0x90000191)
        self.assertEqual(self.call('symbol',top,b'absent',C.byref(out)),-91)
