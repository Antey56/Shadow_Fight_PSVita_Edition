import ctypes as C,struct,unittest
import test_arm_exec as e
import test_arm_link as link
from test_elf_lifecycle import lifecycle
class Plan(C.Structure):
    _fields_=[('init',e.U*1024),('fini',e.U*1024),('init_count',e.Z),('fini_count',e.Z),
              ('base',e.U),('image_size',e.Z),('validated',C.c_int)]
@unittest.skipUnless(e.DLL.exists(),'build arm_exec_host first')
class LifecycleC(unittest.TestCase):
    def setUp(self):
        self.lib=C.CDLL(str(e.DLL.resolve()))
        args=[e.P,e.Z,e.Z,e.Resolver,e.P,C.POINTER(e.Backend),C.POINTER(e.Image),C.POINTER(e.Error)]
        self.lib.arm_exec_map.argtypes=args
        self.lib.arm_exec_map_planned.argtypes=args+[C.POINTER(Plan)]
        self.lib.arm_exec_release.argtypes=[C.POINTER(e.Image)]
        self.lib.arm_link_image.argtypes=[e.P,e.Z,e.U,e.Z,e.Resolver,e.P,C.POINTER(link.Image),C.POINTER(e.Error)]
        self.lib.arm_link_dispose.argtypes=[C.POINTER(link.Image)]
        self.lib.arm_lifecycle_plan.argtypes=[e.P,e.Z,C.POINTER(link.Image),C.POINTER(Plan)]
        self.events=[];self.fail=None;self.images=[];self.plan=Plan();self.resolve=e.Resolver(lambda *a:0)
        def reserve(ctx,size,alignment,base,cookie):
            self.events.append('reserve');base[0]=0x90000000;cookie[0]=5;return 0
        def publish(ctx,cookie,data,size):
            self.events.append('publish');return -71 if self.fail=='publish' else 0
        def sync(ctx,cookie,size):
            self.events.append('sync');return -72 if self.fail=='sync' else 0
        def release(ctx,cookie):self.events.append('release')
        self.callbacks=[e.Reserve(reserve),e.Publish(publish),e.Sync(sync),e.Release(release)]
        self.backend=e.Backend(None,*self.callbacks)
    def tearDown(self):
        for image in self.images:self.lib.arm_exec_release(C.byref(image))
    def map(self,data=None,planned=True):
        data=lifecycle() if data is None else data;buf=C.create_string_buffer(bytes(data));image=e.Image();err=e.Error()
        self.images.append(image)
        args=[buf,len(data),4096,self.resolve,None,C.byref(self.backend),C.byref(image),C.byref(err)]
        if planned:rc=self.lib.arm_exec_map_planned(*args,C.byref(self.plan))
        else:rc=self.lib.arm_exec_map(*args)
        return rc,image
    def empty_plan(self):
        self.assertEqual(bytes(self.plan),bytes(C.sizeof(Plan)))
    def test_opt_in_plan_before_publication(self):
        rc,image=self.map();self.assertEqual(rc,0);self.assertTrue(image.ready);self.assertTrue(self.plan.validated)
        self.assertEqual(list(self.plan.init[:self.plan.init_count]),[0x90000181,0x90000189])
        self.assertEqual(list(self.plan.fini[:self.plan.fini_count]),[0x9000018d,0x90000185])
        self.assertEqual(self.events,['reserve','publish','sync'])
    def test_default_constructor_gate_still_closed(self):
        self.assertEqual(self.map(planned=False)[0],-32);self.assertEqual(self.events,[])
    def test_invalid_last_fini_prevents_any_publication(self):
        data=lifecycle();struct.pack_into('<I',data,512+12*8+4,0x1001)
        self.assertEqual(self.map(data)[0],-104);self.empty_plan();self.assertEqual(self.events,['reserve','release'])
    def test_unrelocated_array_pointer_rejected(self):
        data=lifecycle();struct.pack_into('<I',data,324,0x191)
        self.assertEqual(self.map(data)[0],-103);self.empty_plan();self.assertNotIn('publish',self.events)
    def test_non_relative_array_relocation_rejected(self):
        data=lifecycle();struct.pack_into('<I',data,308,2)
        self.assertEqual(self.map(data)[0],-103);self.empty_plan()
    def test_duplicate_lifecycle_tag(self):
        data=lifecycle();struct.pack_into('<I',data,512+12*8,12)
        self.assertEqual(self.map(data)[0],-101);self.empty_plan()
    def test_missing_size_and_shared_preinit_rejected(self):
        data=lifecycle();struct.pack_into('<I',data,512+8*8,29)
        self.assertEqual(self.map(data)[0],-101);self.empty_plan()
        data=lifecycle();struct.pack_into('<I',data,512+9*8,32);struct.pack_into('<I',data,512+10*8,33)
        self.assertEqual(self.map(data)[0],-101);self.empty_plan()
    def test_capacity_and_alignment(self):
        data=lifecycle();struct.pack_into('<I',data,512+8*8+4,4100)
        self.assertEqual(self.map(data)[0],-102);self.empty_plan()
        data=lifecycle();struct.pack_into('<I',data,320,0x182)
        self.assertEqual(self.map(data)[0],-104);self.empty_plan()
    def test_publish_and_sync_failure_clear_plan_and_release(self):
        for failure,rc in [('publish',-71),('sync',-72)]:
            self.events=[];self.fail=failure
            status,image=self.map();self.assertEqual(status,rc);self.assertFalse(image.ready)
            self.empty_plan();self.assertEqual(self.events[-1],'release')
    def test_tls_is_rejected_even_in_opt_in_mode(self):
        data=lifecycle();headers=bytes(data[52:116]);struct.pack_into('<I',data,28,384);struct.pack_into('<H',data,44,3)
        data[384:448]=headers;struct.pack_into('<IIIIIIII',data,448,7,0,0,0,0,16,4,4)
        self.assertEqual(self.map(data)[0],-32);self.assertEqual(self.events,[]);self.empty_plan()
    def test_staged_relocation_mismatch_is_not_accepted(self):
        data=lifecycle();buf=C.create_string_buffer(bytes(data));image=link.Image();error=e.Error()
        self.assertEqual(self.lib.arm_link_image(buf,len(data),0x90000000,4096,self.resolve,None,C.byref(image),C.byref(error)),0)
        try:
            image.bytes[320]=0
            self.assertEqual(self.lib.arm_lifecycle_plan(buf,len(data),C.byref(image),C.byref(self.plan)),-103)
            self.empty_plan()
        finally:self.lib.arm_link_dispose(C.byref(image))
    def test_bss_and_null_targets_not_executable(self):
        data=lifecycle();struct.pack_into('<I',data,72,1024);struct.pack_into('<I',data,320,0x301)
        self.assertEqual(self.map(data)[0],-104);self.empty_plan()
        data=lifecycle();struct.pack_into('<I',data,320,0)
        self.assertEqual(self.map(data)[0],-104);self.empty_plan()
