import ctypes as C
import os
import pathlib
import sys
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from asset_host import load_backend
DLL = next((p for p in (ROOT/'build-host/jni_objects_host.dll', ROOT/'build-host/jni_objects_host.so')
            if p.exists()), ROOT/'build-host/jni_objects_host.so')


@unittest.skipUnless(DLL.exists(), 'build jni_objects_host first')
class ObjectsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load_backend(DLL)
        P, R, I = C.c_void_p, C.c_uint32, C.c_int
        signatures = {
            'sf_jni_objects_create': ([C.c_char_p, C.c_size_t], P),
            'sf_jni_objects_destroy': ([P], None), 'sf_jni_context': ([P], R),
            'sf_jni_new_ref': ([P, R, I], R), 'sf_jni_delete_ref': ([P, R, I], I),
            'sf_jni_clear_locals': ([P], None), 'sf_jni_exception': ([P], I),
            'sf_jni_exception_clear': ([P], None),
            'sf_jni_context_call': ([P, R, C.c_char_p, C.c_char_p], R),
            'sf_jni_asset_manager': ([P, R], P)}
        for name, (args, result) in signatures.items():
            f = getattr(cls.lib, name); f.argtypes = args; f.restype = result

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT/'tests')
        (pathlib.Path(self.temp.name)/'sample').write_bytes(b'asset-through-context')
        self.e = self.lib.sf_jni_objects_create(os.fsencode(self.temp.name), 4096)
        self.assertTrue(self.e)
        self.context = self.lib.sf_jni_context(self.e)

    def tearDown(self):
        self.lib.sf_jni_objects_destroy(self.e); self.temp.cleanup()

    def assets(self, ref=None, name=b'getAssets', sig=b'()Landroid/content/res/AssetManager;'):
        return self.lib.sf_jni_context_call(self.e, self.context if ref is None else ref, name, sig)

    def test_read_through_context(self):
        manager = self.lib.sf_jni_asset_manager(self.e, self.assets())
        self.assertTrue(manager)
        a = self.lib.AAssetManager_open(manager, b'sample', 2); self.assertTrue(a)
        try:
            buf = C.create_string_buffer(64)
            n = self.lib.AAsset_read(a, buf, 64)
            self.assertEqual(buf.raw[:n], b'asset-through-context')
        finally:
            self.lib.AAsset_close(a)

    def test_wrong_type(self):
        self.assertFalse(self.lib.sf_jni_asset_manager(self.e, self.context))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 2)

    def test_wrong_receiver(self):
        self.assertFalse(self.assets(self.assets()))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 2)

    def test_method_mismatch_and_sticky_exception(self):
        self.assertFalse(self.assets(name=b'getFilesDir'))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 4)
        self.assertFalse(self.assets())
        self.lib.sf_jni_delete_ref(self.e, 0xffffffff, 0)
        self.assertEqual(self.lib.sf_jni_exception(self.e), 4)
        self.lib.sf_jni_exception_clear(self.e)
        self.assertTrue(self.assets())

    def test_signature_mismatch(self):
        self.assertFalse(self.assets(sig=b'()V'))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 4)

    def test_stale_ref_after_slot_reuse(self):
        old = self.context
        self.assertEqual(self.lib.sf_jni_delete_ref(self.e, old, 0), 0)
        new = self.lib.sf_jni_context(self.e); self.assertNotEqual(old, new)
        self.assertFalse(self.assets(old))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 1)

    def test_global_survives_local_clear(self):
        g = self.lib.sf_jni_new_ref(self.e, self.context, 1)
        self.lib.sf_jni_clear_locals(self.e)
        self.assertTrue(self.assets(g))
        self.assertEqual(self.lib.sf_jni_delete_ref(self.e, g, 1), 0)
        self.assertFalse(self.assets(g))

    def test_delete_kind_mismatch_preserves_ref(self):
        self.assertEqual(self.lib.sf_jni_delete_ref(self.e, self.context, 1), -1)
        self.lib.sf_jni_exception_clear(self.e)
        self.assertTrue(self.assets())

    def test_capacity_and_recovery(self):
        for _ in range(63): self.assertTrue(self.lib.sf_jni_context(self.e))
        self.assertFalse(self.lib.sf_jni_context(self.e))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 3)
        self.lib.sf_jni_clear_locals(self.e); self.lib.sf_jni_exception_clear(self.e)
        self.assertTrue(self.lib.sf_jni_context(self.e))

    def test_null_and_invalid(self):
        self.assertEqual(self.lib.sf_jni_delete_ref(self.e, 0, 0), 0)
        self.assertFalse(self.lib.sf_jni_new_ref(self.e, 0, 1))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 0)
        self.assertFalse(self.assets(0xffffffff))
        self.assertEqual(self.lib.sf_jni_exception(self.e), 1)

    def test_global_asset_copy(self):
        a = self.assets(); g = self.lib.sf_jni_new_ref(self.e, a, 1)
        m = self.lib.sf_jni_asset_manager(self.e, a)
        self.lib.sf_jni_clear_locals(self.e)
        self.assertEqual(self.lib.sf_jni_asset_manager(self.e, g), m)

    def test_deleted_local_copy_independent(self):
        copy = self.lib.sf_jni_new_ref(self.e, self.context, 0)
        self.lib.sf_jni_delete_ref(self.e, self.context, 0)
        self.assertTrue(self.assets(copy))
