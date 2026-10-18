import pathlib,struct,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).parents[1]
EXE=next((p for p in [ROOT/'build-host/elf_preflight',ROOT/'build-host/elf_preflight.exe',ROOT/'tools/elf_preflight.exe'] if p.exists()),ROOT/'build-host/elf_preflight')
def fixture():
    h=struct.pack('<16sHHIIIIIHHHHHH',b'\x7fELF\x01\x01\x01'+b'\0'*9,3,40,1,0,52,0,0x5000200,52,32,2,40,0,0)
    load=struct.pack('<IIIIIIII',1,0,0,0,116,128,5,4096)
    dynamic=struct.pack('<IIIIIIII',2,116,116,116,0,0,6,4)
    return bytearray(h+load+dynamic)
@unittest.skipUnless(EXE.exists(),'build host elf_preflight executable first')
class ElfTests(unittest.TestCase):
    def inspect(self,b):
        with tempfile.TemporaryDirectory(dir=ROOT/'tests') as d:
            p=pathlib.Path(d)/'fixture.so';p.write_bytes(b)
            return subprocess.run([str(EXE.resolve()),str(p.resolve())],capture_output=True,text=True)
    def test_valid(self):self.assertEqual(self.inspect(fixture()).returncode,0)
    def test_truncated(self):self.assertNotEqual(self.inspect(fixture()[:51]).returncode,0)
    def test_hardfloat(self):
        b=fixture();struct.pack_into('<I',b,36,0x5000400)
        self.assertIn('hard-float',self.inspect(b).stdout)
    def test_arm64(self):
        b=fixture();b[4]=2;struct.pack_into('<H',b,18,183)
        self.assertNotEqual(self.inspect(b).returncode,0)
    def test_out_of_file(self):
        b=fixture();struct.pack_into('<I',b,52+16,10000)
        self.assertNotEqual(self.inspect(b).returncode,0)
    def test_alignment(self):
        b=fixture();struct.pack_into('<I',b,52+28,3)
        self.assertNotEqual(self.inspect(b).returncode,0)
    def test_program_table_overflow(self):
        b=fixture();struct.pack_into('<I',b,28,0xfffffff0)
        self.assertNotEqual(self.inspect(b).returncode,0)
