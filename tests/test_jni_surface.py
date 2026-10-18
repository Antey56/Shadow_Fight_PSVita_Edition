import pathlib
import struct
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from jni_surface import method_references


def fixture(cls='Landroid/content/Context;', name='getAssets', ret='Landroid/content/res/AssetManager;'):
    strings = [cls, ret, name]
    b = bytearray(112)
    b[:8] = b'dex\n035\0'
    def put(offset, value):
        struct.pack_into('<I', b, offset, value)
    put(56, 3); put(60, len(b)); string_ids = len(b); b.extend(bytes(12))
    put(64, 2); put(68, len(b)); b.extend(struct.pack('<II', 0, 1))
    put(72, 1); put(76, len(b)); b.extend(struct.pack('<III', 0, 1, 0))
    put(88, 1); put(92, len(b)); b.extend(struct.pack('<HHI', 0, 0, 2))
    for i, s in enumerate(strings):
        put(string_ids+i*4, len(b)); b.extend(bytes([len(s)])+s.encode()+b'\0')
    return b


class SurfaceTests(unittest.TestCase):
    def test_asset_signature(self):
        self.assertEqual(method_references(fixture()), [{'class': 'Landroid/content/Context;',
            'method': 'getAssets', 'descriptor': '()Landroid/content/res/AssetManager;'}])

    def test_unrelated_class_excluded(self):
        self.assertEqual(method_references(fixture(cls='Lexample/Private;')), [])

    def test_unrelated_method_excluded(self):
        self.assertEqual(method_references(fixture(name='privateToken')), [])

    def test_array_return_retained(self):
        self.assertEqual(method_references(fixture(name='list', ret='[Ljava/lang/String;'))[0]['descriptor'],
                         '()[Ljava/lang/String;')

    def test_bad_magic(self):
        with self.assertRaises(ValueError):
            method_references(bytes(112))
