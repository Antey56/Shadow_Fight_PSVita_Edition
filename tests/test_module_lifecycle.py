import ctypes as C,unittest
import test_arm_namespace as n
from test_arm_lifecycle import Plan
Invoke=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.c_size_t)
class Runner(C.Structure):
    _fields_=[('ns',C.POINTER(n.Namespace)),('handle',n.U),('image',C.POINTER(n.Image)),('plan',Plan),
              ('invoke',Invoke),('context',C.c_void_p),('init_attempted',n.Z),('init_completed',n.Z),
              ('fini_attempted',n.Z),('fini_completed',n.Z),('state',C.c_uint)]
@unittest.skipUnless(n.DLL.exists(),'build arm_namespace_host first')
class ModuleLifecycleTests(unittest.TestCase):
    call=n.NamespaceTests.call
    add=n.NamespaceTests.add
    ready=n.NamespaceTests.ready
    def setUp(self):
        n.NamespaceTests.setUp(self);self.runner=Runner();self.events=[];self.fail_at=None;self.reenter=False;self.reentry_results=[]
        self.lib.sf_module_lifecycle_bind.argtypes=[C.POINTER(Runner),C.POINTER(n.Namespace),n.U,C.POINTER(n.Image),C.POINTER(Plan),Invoke,C.c_void_p]
        self.lib.sf_module_initialize.argtypes=[C.POINTER(Runner)]
        self.lib.sf_module_finalize.argtypes=[C.POINTER(Runner)]
        def invoke(ctx,address,phase,index):
            self.events.append((phase,index,address))
            if self.reenter:
                self.reentry_results.append((self.lib.sf_module_initialize(C.byref(self.runner)),self.lib.sf_module_finalize(C.byref(self.runner))))
            return -500 if (phase,index)==self.fail_at else 0
        self.callback=Invoke(invoke)
    def prepare(self,deps=()):
        rc,h=self.add(deps=deps);self.assertEqual(rc,0);image=self.keep[-1][1]
        image.code_count=1;image.code[0].begin=image.base;image.code[0].end=image.base+512
        self.plan=Plan();self.plan.base=image.base;self.plan.image_size=image.size;self.plan.validated=1
        self.plan.init_count=2;self.plan.init[0]=image.base+0x181;self.plan.init[1]=image.base+0x185
        self.plan.fini_count=2;self.plan.fini[0]=image.base+0x18d;self.plan.fini[1]=image.base+0x189
        self.handle=h;self.image=image
    def bind(self):
        return self.lib.sf_module_lifecycle_bind(C.byref(self.runner),C.byref(self.ns),self.handle,C.byref(self.image),C.byref(self.plan),self.callback,None)
    def initialize(self):return self.lib.sf_module_initialize(C.byref(self.runner))
    def finalize(self):return self.lib.sf_module_finalize(C.byref(self.runner))
    def test_order_one_shot_and_copy(self):
        self.prepare();self.assertEqual(self.bind(),0);base=self.image.base
        self.plan.init[0]=0 # original caller plan cannot mutate controller copy
        self.assertEqual(self.initialize(),0);self.assertEqual(self.call('state',self.handle),3)
        self.assertEqual(self.initialize(),-112);self.assertEqual(self.finalize(),0)
        self.assertEqual(self.finalize(),-112);self.assertEqual(self.initialize(),-112)
        self.assertEqual(self.events,[(0,0,base+0x181),(0,1,base+0x185),(1,0,base+0x18d),(1,1,base+0x189)])
        self.assertEqual(self.runner.init_completed,2);self.assertEqual(self.runner.fini_completed,2)
    def test_partial_init_failure_poisoned_no_retry_or_teardown(self):
        _,parent=self.add(b'libparent.so');self.ready(parent)
        self.prepare((parent,));self.assertEqual(self.bind(),0);self.fail_at=(0,1)
        self.assertEqual(self.initialize(),-500);self.assertEqual(self.runner.state,6)
        self.assertEqual(self.runner.init_attempted,2);self.assertEqual(self.runner.init_completed,1)
        self.assertEqual(self.call('state',self.handle),5)
        self.assertEqual(self.initialize(),-112);self.assertEqual(self.finalize(),-112)
        self.assertEqual(len(self.events),2);self.assertEqual(self.call('begin_fini',parent),-92)
        self.assertTrue(self.image.ready) # no automatic release/rollback
    def test_fini_failure_stops_remaining_calls(self):
        self.prepare();self.bind();self.assertEqual(self.initialize(),0);self.fail_at=(1,0)
        self.assertEqual(self.finalize(),-500);self.assertEqual(self.runner.fini_attempted,1)
        self.assertEqual(self.runner.fini_completed,0);self.assertEqual(self.finalize(),-112)
        self.assertEqual(len(self.events),3);self.assertTrue(self.image.ready)
    def test_dependencies_not_ready_cannot_start(self):
        _,parent=self.add(b'libparent.so');self.prepare((parent,));self.bind()
        self.assertEqual(self.initialize(),-95);self.assertEqual(self.events,[])
        self.ready(parent);self.assertEqual(self.initialize(),0)
    def test_reentrant_calls_rejected(self):
        self.prepare();self.bind();self.reenter=True
        self.assertEqual(self.initialize(),0);self.assertEqual(self.finalize(),0)
        self.assertEqual(self.reentry_results,[(-112,-112)]*4)
    def test_open_handles_and_dependency_pins_block_finalization(self):
        self.prepare();self.bind();self.initialize();opened=n.U()
        self.assertEqual(self.call('open',b'liba.so',C.byref(opened)),0)
        self.assertEqual(self.finalize(),-92);self.assertEqual(len(self.events),2)
        self.call('close',opened);_,child=self.add(b'libchild.so',(self.handle,))
        self.assertEqual(self.finalize(),-92);self.call('remove',child)
        self.assertEqual(self.finalize(),0)
    def test_entire_fini_plan_checked_before_first_init(self):
        self.prepare();self.plan.fini[1]=self.image.base+0x1001
        self.assertEqual(self.bind(),-111);self.assertEqual(self.initialize(),-112)
        self.assertEqual(self.events,[]);self.assertEqual(self.call('state',self.handle),1)
    def test_stale_mapping_and_unvalidated_plan(self):
        self.prepare();self.plan.validated=0;self.assertEqual(self.bind(),-110)
        self.plan.validated=1;self.assertEqual(self.bind(),0)
        self.call('remove',self.handle);self.assertEqual(self.initialize(),-112);self.assertEqual(self.events,[])
    def test_empty_plan_transitions_without_calls(self):
        self.prepare();self.plan.init_count=0;self.plan.fini_count=0
        self.assertEqual(self.bind(),0);self.assertEqual(self.initialize(),0);self.assertEqual(self.finalize(),0)
        self.assertEqual(self.events,[]);self.assertEqual(self.call('state',self.handle),6)
