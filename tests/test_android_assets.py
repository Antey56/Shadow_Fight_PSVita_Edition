import pathlib,sys,unittest,tempfile,ctypes as C,os
ROOT=pathlib.Path(__file__).parents[1];sys.path.insert(0,str(ROOT/'tools'))
from asset_host import load_backend
DLL=next((p for p in [ROOT/'build-host/android_asset_host.dll',ROOT/'build-host/android_asset_host.so'] if p.exists()),ROOT/'build-host/android_asset_host.so')
@unittest.skipUnless(DLL.exists(),'build android_asset_host first')
class AssetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.lib=load_backend(DLL)
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(dir=ROOT/'tests');self.root=pathlib.Path(self.temp.name).resolve()
        self.data=b'0123456789'+bytes(range(256));(self.root/'nested').mkdir();(self.root/'nested/data.bin').write_bytes(self.data);(self.root/'empty').write_bytes(b'')
        self.manager=self.lib.sf_asset_manager_create(os.fsencode(self.root),4096);self.assertTrue(self.manager);self.assets=[]
    def tearDown(self):
        for a in self.assets:self.lib.AAsset_close(a)
        if self.manager:self.lib.sf_asset_manager_destroy(self.manager)
        self.temp.cleanup()
    def opened(self,name=b'nested/data.bin',mode=2):
        a=self.lib.AAssetManager_open(self.manager,name,mode)
        if a:self.assets.append(a)
        return a
    def read(self,a,count):
        buf=C.create_string_buffer(count);n=self.lib.AAsset_read(a,buf,count)
        return n,buf.raw[:max(0,n)]
    def test_stream(self):
        a=self.opened();parts=[]
        while True:
            n,b=self.read(a,17)
            if not n:break
            self.assertGreater(n,0);parts.append(b)
        self.assertEqual(b''.join(parts),self.data)
    def test_lengths(self):
        a=self.opened();self.assertEqual(self.lib.AAsset_getLength(a),len(self.data));self.assertEqual(self.lib.AAsset_getLength64(a),len(self.data));self.read(a,10)
        self.assertEqual(self.lib.AAsset_getRemainingLength(a),len(self.data)-10);self.assertEqual(self.lib.AAsset_getRemainingLength64(a),len(self.data)-10)
    def test_seek_variants(self):
        a=self.opened();self.assertEqual(self.lib.AAsset_seek(a,5,0),5);self.assertEqual(self.read(a,3)[1],b'567')
        self.assertEqual(self.lib.AAsset_seek64(a,-2,2),len(self.data)-2);self.assertEqual(self.read(a,8)[1],self.data[-2:]);self.assertEqual(self.read(a,1)[0],0)
    def test_invalid_seek_preserves_position(self):
        a=self.opened();self.lib.AAsset_seek(a,4,0)
        for offset,whence in [(-5,1),(9999,0),(-(1<<63),1),((1<<63)-1,1),(0,9)]:
            self.assertEqual(self.lib.AAsset_seek64(a,offset,whence),-1)
        self.assertEqual(self.read(a,1)[1],b'4')
    def test_independent_handles(self):
        a=self.opened();b=self.opened();self.read(a,5);self.assertEqual(self.read(b,1)[1],b'0');self.assertEqual(self.read(a,1)[1],b'5')
    def test_lazy_buffer(self):
        a=self.opened(mode=3);self.assertEqual(self.lib.AAsset_isAllocated(a),0);self.lib.AAsset_seek(a,7,0)
        p=self.lib.AAsset_getBuffer(a);self.assertTrue(p);self.assertEqual(C.string_at(p,len(self.data)),self.data);self.assertEqual(self.lib.AAsset_getBuffer(a),p)
        self.assertEqual(self.lib.AAsset_isAllocated(a),1);self.assertEqual(self.read(a,1)[1],b'7')
    def test_buffer_limit_stream_still_works(self):
        self.lib.sf_asset_manager_destroy(self.manager);self.manager=self.lib.sf_asset_manager_create(os.fsencode(self.root),1)
        a=self.opened();self.assertFalse(self.lib.AAsset_getBuffer(a));self.assertEqual(self.read(a,10)[1],b'0123456789')
    def test_empty(self):
        a=self.opened(b'empty');self.assertTrue(a);self.assertEqual(self.lib.AAsset_getLength64(a),0);self.assertEqual(self.read(a,1)[0],0);self.assertTrue(self.lib.AAsset_getBuffer(a))
    def test_traversal_rejected(self):
        for path in [b'',b'..',b'.',b'../outside',b'nested/../empty',b'/etc/passwd',b'C:/file',b'ux0:data/file',b'nested\\data.bin',b'nested//data.bin',b'nested/./data.bin',b'bad\x01name',b'a'*1024]:
            with self.subTest(path=path):self.assertFalse(self.opened(path))
    def test_missing_directory_and_mode(self):
        self.assertFalse(self.opened(b'missing'));self.assertFalse(self.opened(b'nested'));self.assertFalse(self.opened(mode=4))
    def test_all_mode_hints(self):
        for mode in range(4):self.assertEqual(self.read(self.opened(mode=mode),1)[1],b'0')
    def test_zero_read_and_null_buffer(self):
        a=self.opened();self.assertEqual(self.lib.AAsset_read(a,None,0),0);self.assertEqual(self.lib.AAsset_read(a,None,1),-1)
    def test_manager_lifetime(self):
        a=self.opened();self.lib.sf_asset_manager_destroy(self.manager);self.manager=None;self.assertEqual(self.read(a,1)[1],b'0')
    def test_host_catalog_refused(self):self.assertEqual(self.lib.sf_asset_target_catalog(None,0),-22)
    def test_null_handles(self):
        self.assertEqual(self.lib.AAsset_getLength64(None),-1);self.assertEqual(self.lib.AAsset_seek64(None,0,0),-1);self.assertFalse(self.lib.AAsset_getBuffer(None));self.lib.AAsset_close(None)
    def test_invalid_manager_config(self):
        self.assertFalse(self.lib.sf_asset_manager_create(None,4096));self.assertFalse(self.lib.sf_asset_manager_create(b'',4096));self.assertFalse(self.lib.sf_asset_manager_create(os.fsencode(self.root),65*1024**2))
