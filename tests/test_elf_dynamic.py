import pathlib,sys,struct,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
from elf_dynamic import DynamicELF
def fixture():
    b=bytearray(512)
    struct.pack_into('<16sHHIIIIIHHHHHH',b,0,b'\x7fELF\x01\x01\x01'+b'\0'*9,3,40,1,0,52,0,0x5000200,52,32,2,0,0,0)
    struct.pack_into('<IIIIIIII',b,52,1,0,0,0,512,512,6,4096)
    struct.pack_into('<IIIIIIII',b,84,2,116,116,116,64,64,6,4)
    tags=[(5,256),(10,16),(6,272),(11,16),(17,304),(18,8),(19,8),(0,0)]
    for i,(t,v) in enumerate(tags):struct.pack_into('<II',b,116+i*8,t,v)
    struct.pack_into('<II',b,304,320,23)
    return b
class DynamicTests(unittest.TestCase):
    def test_no_sections(self):
        r=DynamicELF(fixture()).report();self.assertEqual(r['relocation_counts'],{'23':1});self.assertFalse(r['section_headers_used'])
    def test_bad_pointer(self):
        b=fixture();struct.pack_into('<I',b,120,4096)
        with self.assertRaises(ValueError):DynamicELF(b)
    def test_missing_terminator(self):
        b=fixture();struct.pack_into('<II',b,172,1,0)
        with self.assertRaises(ValueError):DynamicELF(b)
    def test_duplicate_tag(self):
        b=fixture();struct.pack_into('<I',b,124,5)
        with self.assertRaises(ValueError):DynamicELF(b)
    def test_bad_target(self):
        b=fixture();struct.pack_into('<I',b,304,510)
        with self.assertRaises(ValueError):DynamicELF(b).report()
    def test_packed_rejected(self):
        b=fixture();struct.pack_into('<I',b,148,0x6000000f)
        with self.assertRaises(ValueError):DynamicELF(b).report()
    def test_truncated(self):
        with self.assertRaises(ValueError):DynamicELF(fixture()[:100])
