import importlib.util,pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
from deep_audit import uleb,category,native_methods
class DeepTests(unittest.TestCase):
    def test_uleb(self):self.assertEqual(uleb(bytes([0xe5,0x8e,0x26]),0),(624485,3))
    def test_bad_uleb(self):
        with self.assertRaises(ValueError):uleb(b'\xff'*5,0)
    def test_categories(self):
        self.assertEqual(category('__sF'),'bionic_data_or_tls')
        self.assertEqual(category('ANativeWindow_fromSurface'),'android')
        self.assertEqual(category('pthread_create'),'threads')
        self.assertEqual(category('eglMakeCurrent'),'egl')
    def test_invalid_dex(self):
        with self.assertRaises(ValueError):native_methods(b'not dex')
