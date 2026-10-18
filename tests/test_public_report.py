import sys,pathlib,unittest,json
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
from public_report import public_report
class PublicationTests(unittest.TestCase):
    def test_private_strings_removed(self):
        r={'apk':{'sha256':'digest'},'summary':{},'manifest':[{'tag':'meta-data','attributes':{'name':'token','value':'DO_NOT_PUBLISH'}},{'tag':'manifest','attributes':{'package':'example','versionName':'1','extra':'DO_NOT_PUBLISH'}}],'libraries':[{'path':'x','imports':[],'interesting_strings':['DO_NOT_PUBLISH'],'exports':['DO_NOT_PUBLISH']}],'dex':[{'path':'classes.dex','string_count':3,'interesting_strings':['DO_NOT_PUBLISH']}],'errors':[]}
        p=public_report(r)
        self.assertNotIn('DO_NOT_PUBLISH',json.dumps(p));self.assertEqual(p['manifest'][0]['attributes']['package'],'example')
