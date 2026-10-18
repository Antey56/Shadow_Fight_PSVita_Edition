import pathlib,struct,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
from elf_lifecycle import plan
from test_elf_dynamic import fixture

def lifecycle():
    b=fixture();struct.pack_into('<I',b,76,5)
    # Move/expand dynamic table into unused space, retaining the LOAD.
    tags=[(5,256),(10,16),(6,272),(11,16),(17,304),(18,16),(19,8),
          (25,320),(27,8),(26,328),(28,8),(12,0x181),(13,0x185),(0,0)]
    b.extend(bytes(128));struct.pack_into('<II',b,68,len(b),len(b))
    struct.pack_into('<IIIIIIII',b,84,2,512,512,512,len(tags)*8,len(tags)*8,4,4)
    for i,pair in enumerate(tags):struct.pack_into('<II',b,512+i*8,*pair)
    struct.pack_into('<IIII',b,304,320,23,328,23)
    struct.pack_into('<IIII',b,320,0x189,0,0x18d,0xffffffff)
    return b
class LifecycleTests(unittest.TestCase):
    def test_order_and_no_execution(self):
        r=plan(lifecycle());self.assertFalse(r['execution_allowed'])
        self.assertEqual([x['vaddr'] for x in r['init_order']],['0x181','0x189'])
        self.assertEqual([x['vaddr'] for x in r['fini_order']],['0x18d','0x185'])
        self.assertEqual(r['init_order'][1]['runtime'],'0x90000189')
    def test_non_executable(self):
        b=lifecycle();struct.pack_into('<I',b,76,6)
        with self.assertRaises(ValueError):plan(b)
    def test_outside_file(self):
        b=lifecycle();struct.pack_into('<I',b,320,0x1001)
        with self.assertRaises(ValueError):plan(b)
    def test_symbol_relocation_rejected(self):
        b=lifecycle();struct.pack_into('<I',b,308,0x102)
        with self.assertRaises(ValueError):plan(b)
    def test_unrelocated_pointer_rejected(self):
        b=lifecycle();struct.pack_into('<I',b,324,0x191)
        with self.assertRaises(ValueError):plan(b)
    def test_incomplete_array(self):
        b=lifecycle();struct.pack_into('<I',b,512+8*8,29)
        with self.assertRaises(ValueError):plan(b)
    def test_alignment_and_reservation(self):
        with self.assertRaises(ValueError):plan(lifecycle(),1)
        b=lifecycle();struct.pack_into('<I',b,320,0x182)
        with self.assertRaises(ValueError):plan(b)
    def test_fini_array_reverse(self):
        b=lifecycle();struct.pack_into('<I',b,308,23)
        struct.pack_into('<I',b,312,332);struct.pack_into('<I',b,332,0x191)
        # Slot 328 becomes a null sentinel; slot 332 is relocated.
        struct.pack_into('<I',b,328,0)
        self.assertEqual([x['vaddr'] for x in plan(b)['fini_order']],['0x191','0x185'])

    def test_two_fini_calls_reverse(self):
        b=lifecycle()
        struct.pack_into('<I',b,512+4*8+4,400)
        struct.pack_into('<I',b,512+5*8+4,24)
        struct.pack_into('<IIIIII',b,400,320,23,328,23,332,23)
        struct.pack_into('<I',b,332,0x191)
        self.assertEqual([x['vaddr'] for x in plan(b)['fini_order']],['0x191','0x18d','0x185'])
