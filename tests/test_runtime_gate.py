import pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
from check_runtime_gate import check
class GateTests(unittest.TestCase):
    def test_missing_and_weak(self):
        r={'libraries':[{'path':'lib/armeabi-v7a/a.so','bits':32,'machine':40,'exports':[],'needed':['libc.so'],'imports':[{'name':'malloc','weak':False},{'name':'optional','weak':True}]}]}
        g=check(r,{})
        self.assertEqual(g['unique_missing_required'],['malloc'])
        self.assertEqual(g['modules'][0]['missing_weak_imports'],['optional'])
        self.assertFalse(check(r,{'malloc':'adapter'})['execution_allowed'])
    def test_internal_provider(self):
        r={'libraries':[{'path':'a.so','bits':32,'machine':40,'exports':[],'needed':['b.so'],'imports':[{'name':'foo','weak':False}]},{'path':'b.so','bits':32,'machine':40,'exports':['foo'],'needed':[],'imports':[]}]}
        g=check(r,{})
        self.assertEqual(g['missing_required_records'],0)
        self.assertEqual(g['modules'][0]['internal_candidates'][0]['candidate_providers'],['b.so'])
        self.assertFalse(g['execution_allowed'])
