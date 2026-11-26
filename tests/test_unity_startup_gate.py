import pathlib,sys,struct,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_unity_bootstrap import thumb_bl_target,registration_layout
from unity_startup_gate import referenced_imports,category
from elf_dynamic import DynamicELF
from test_arm_link import external_fixture
from test_elf_dynamic import fixture
class StartupGateTests(unittest.TestCase):
    def test_forward_and_backward_thumb_bl(self):
        self.assertEqual(thumb_bl_target(0xf000,0xf820,0x26ca3c),0x26ca80)
        self.assertEqual(thumb_bl_target(0xf733,0xf9bb,0x26ca1e),0x19fd98)
        with self.assertRaises(ValueError):thumb_bl_target(0,0,0)
    def test_required_and_weak_are_distinct(self):
        self.assertEqual(referenced_imports(DynamicELF(external_fixture()))['required'],['foo'])
        r=referenced_imports(DynamicELF(external_fixture(2)))
        self.assertEqual(r['required'],[]);self.assertEqual(r['weak'],['foo'])
    def test_defined_symbol_is_not_an_external_import(self):
        b=external_fixture();struct.pack_into('<H',b,302,1)
        self.assertEqual(referenced_imports(DynamicELF(b))['required'],[])
    def test_registration_pattern_is_checked(self):
        b=fixture();v=0x180
        for off,value in [(0,0xb5b0),(2,0x4d0e),(8,0x447d),(0x18,0x2303),(0x20,0x4a07),(0x22,0x447a),(0x24,0x47e0)]:
            struct.pack_into('<H',b,v+off,value)
        b[v+0x1a:v+0x1e]=bytes.fromhex('d0f85cc3')
        r=registration_layout(DynamicELF(b),v)
        self.assertEqual(r['count'],3);self.assertEqual(r['clit'],0x1bc);self.assertEqual(r['tlit'],0x1c0)
        b[v+0x1a]=0
        with self.assertRaises(ValueError):registration_layout(DynamicELF(b),v)
    def test_categories_do_not_claim_implementation(self):
        self.assertEqual(category('pthread_create'),'threads_and_synchronization')
        self.assertEqual(category('eglSwapBuffers'),'egl_surface_and_graphics')
        self.assertEqual(category('clock_gettime'),'time')
