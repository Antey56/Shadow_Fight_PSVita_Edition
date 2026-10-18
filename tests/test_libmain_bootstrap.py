import pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_libmain_bootstrap import audit
class MainBootstrapTests(unittest.TestCase):
    def test_rejects_other_binary_before_addresses(self):
        with self.assertRaisesRegex(ValueError,'SHA-256 mismatch'):audit(b'other',[])
