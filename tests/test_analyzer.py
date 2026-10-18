import importlib.util, pathlib, tempfile, unittest, zipfile, struct
spec=importlib.util.spec_from_file_location('analyzer',pathlib.Path(__file__).parents[1]/'tools/analyze_apk.py')
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
class AnalyzerTests(unittest.TestCase):
    def test_plain_manifest(self):
        tags=a.manifest(b'<manifest package="example"><uses-sdk minSdkVersion="21"/></manifest>')
        self.assertEqual(tags[0]['attributes']['package'],'example')
    def test_elf32_gate(self):
        b=struct.pack('<16sHHIIIIIHHHHHH',b'\x7fELF\x01\x01'+b'\0'*10,3,40,1,0,0,0,0,52,32,0,40,0,0)
        e=a.elf(b);self.assertEqual((e['bits'],e['machine']),(32,40))
    def test_path_escape_rejected(self):
        with tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).parent) as d:
            p=pathlib.Path(d)/'x.apk'
            with zipfile.ZipFile(p,'w') as z:z.writestr('assets/../../bad','x')
            with self.assertRaises(ValueError):a.analyze(p)
    def test_bad_elf_reports_error(self):
        with tempfile.TemporaryDirectory(dir=pathlib.Path(__file__).parent) as d:
            p=pathlib.Path(d)/'x.apk'
            with zipfile.ZipFile(p,'w') as z:
                z.writestr('AndroidManifest.xml','<manifest package="test"/>')
                z.writestr('lib/armeabi-v7a/libmain.so','broken')
                z.writestr('assets/users.xml','<users/>')
            r=a.analyze(p)
            self.assertEqual(len(r['errors']),1)
            self.assertFalse(r['summary']['armv7_present'])
            self.assertEqual(r['assets'][0]['kind'],'config_candidate')
if __name__=='__main__':unittest.main()
