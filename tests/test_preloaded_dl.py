import ctypes as C,unittest,struct
import test_arm_namespace as n
class Dl(C.Structure):
    _fields_=[('ns',C.POINTER(n.Namespace)),('error',C.c_char*128),('pending',C.c_uint)]
class DlTests(unittest.TestCase):
    call=n.NamespaceTests.call
    add=n.NamespaceTests.add
    ready=n.NamespaceTests.ready
    def setUp(self):
        n.NamespaceTests.setUp(self)
        self.dl=Dl()
        self.lib.sf_preloaded_dl_init.argtypes=[C.POINTER(Dl),C.POINTER(n.Namespace)]
        self.lib.sf_preloaded_dlopen.argtypes=[C.POINTER(Dl),C.c_char_p,C.c_int];self.lib.sf_preloaded_dlopen.restype=n.U
        self.lib.sf_preloaded_dlsym.argtypes=[C.POINTER(Dl),n.U,C.c_char_p];self.lib.sf_preloaded_dlsym.restype=n.U
        self.lib.sf_preloaded_dlclose.argtypes=[C.POINTER(Dl),n.U]
        self.lib.sf_preloaded_dlerror.argtypes=[C.POINTER(Dl)];self.lib.sf_preloaded_dlerror.restype=C.c_char_p
        self.lib.sf_preloaded_dl_init(C.byref(self.dl),C.byref(self.ns))
    def dlcall(self,name,*args):return getattr(self.lib,'sf_preloaded_dl'+name)(C.byref(self.dl),*args)
    def test_open_symbol_close_real_export(self):
        _,h=self.add();self.ready(h)
        self.assertEqual(self.dlcall('open',b'liba.so',0),h)
        self.assertEqual(self.dlcall('sym',h,b'foo'),0x90000191)
        self.assertIsNone(self.dlcall('error'))
        self.assertEqual(self.dlcall('close',h),0)
        self.assertEqual(self.dlcall('sym',h,b'foo'),0)
        self.assertIn(b'not open',self.dlcall('error'));self.assertIsNone(self.dlcall('error'))
    def test_arm32_flags_only(self):
        _,h=self.add();self.ready(h)
        for flags in (0,1):
            self.assertEqual(self.dlcall('open',b'liba.so',flags),h);self.assertEqual(self.dlcall('close',h),0)
        for flags in (2,3,0x100,0x1000,-1):
            self.assertEqual(self.dlcall('open',b'liba.so',flags),0)
            self.assertIn(b'flags',self.dlcall('error'))
        self.assertEqual(self.ns.modules[0].references,0)
    def test_not_ready_and_missing_name_are_real_errors(self):
        self.add();self.assertEqual(self.dlcall('open',b'liba.so',0),0)
        self.assertIn(b'not ready',self.dlcall('error'))
        self.assertEqual(self.dlcall('open',b'libmissing.so',0),0)
        self.assertIn(b'not registered',self.dlcall('error'))
    def test_no_namespace_and_invalid_close(self):
        self.lib.sf_preloaded_dl_init(C.byref(self.dl),None)
        self.assertEqual(self.dlcall('open',b'liba.so',0),0)
        self.assertIn(b'unavailable',self.dlcall('error'))
        self.assertEqual(self.dlcall('close',1),-1)
        self.assertIn(b'unavailable',self.dlcall('error'))
    def test_stale_handles_and_paths_rejected(self):
        _,old=self.add();self.call('remove',old);_,h=self.add();self.ready(h)
        self.assertEqual(self.dlcall('open',b'liba.so',1),h)
        self.assertEqual(self.dlcall('close',old),-1)
        self.assertIn(b'invalid module handle',self.dlcall('error'))
        self.assertEqual(self.ns.modules[0].references,1)
        self.assertEqual(self.dlcall('open',b'/data/liba.so',0),0)
        self.assertIn(b'name',self.dlcall('error'))
    def test_zero_valued_absolute_export_is_not_lookup_failure(self):
        data=n.symbols();data[300]=0x11
        struct.pack_into('<I',data,292,0);struct.pack_into('<H',data,302,0xfff1)
        _,h=self.add(data=data);self.ready(h);self.dlcall('open',b'liba.so',0)
        self.assertEqual(self.dlcall('sym',h,b'foo'),0);self.assertIsNone(self.dlcall('error'))
