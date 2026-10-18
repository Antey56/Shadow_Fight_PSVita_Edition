import pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
from plan_memory import plan
def lib(name,segs):return {'path':name,'bits':32,'machine':40,'load_segments':[{'vaddr':a,'memsz':b} for a,b in segs]}
class MemoryTests(unittest.TestCase):
    def test_independent_slots(self):
        r=plan([lib('a',[(0,5000)]),lib('b',[(0,100)])],65536)
        self.assertTrue(r['layout_valid']);self.assertEqual(r['modules'][1]['arena_offset'],12288)
        self.assertFalse(r['execution_allowed'])
    def test_nonzero_virtual_base(self):
        r=plan([lib('a',[(8192,4096)])],65536)
        self.assertEqual(r['modules'][0]['segments'][0]['arena_offset'],0)
    def test_overlap(self):self.assertFalse(plan([lib('a',[(0,5000),(4096,5000)])],65536)['layout_valid'])
    def test_shared_page(self):self.assertTrue(plan([lib('a',[(0,200),(200,100)])],65536)['layout_valid'])
    def test_budget(self):self.assertFalse(plan([lib('a',[(0,8192)])],4096)['layout_valid'])
    def test_overflow(self):
        with self.assertRaises(ValueError):plan([lib('a',[(0xffffffff,5)])],65536)
    def test_wrong_arch(self):
        l=lib('a',[(0,1)]);l['bits']=64
        self.assertFalse(plan([l],65536)['layout_valid'])
    def test_bad_page(self):
        with self.assertRaises(ValueError):plan([],65536,page=3000)
    def test_segment_alignment(self):
        l=lib('b',[(0,100)]);l['load_segments'][0]['align']=65536
        r=plan([lib('a',[(0,100)]),l],200000)
        self.assertEqual(r['modules'][1]['arena_offset'],65536)
