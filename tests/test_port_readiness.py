import pathlib
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from assess_port_readiness import assess
from audit_unity_bootstrap import audit


class ReadinessTests(unittest.TestCase):
    def report(self,slots=()):
        return assess({'modules':[{'name':'libunity.so','missing_required_imports':['eglGetDisplay','malloc'],
                                  'external_dependencies':['libEGL.so']}],
                       'unique_missing_required':['eglGetDisplay','malloc']},
                      {'dynamic_api_strings':{'libunity.so':['glDrawArrays','vkCreateInstance']}},
                      {'observed_jni_slots':[113,215,219],'confirmed_method_count':28},
                      {'implemented_slots':list(slots)})

    def test_missing_slots(self):
        self.assertEqual(self.report([219])['stages'][1]['observed_missing_slots'],[113,215])

    def test_no_execution_even_all_observed_slots_implemented(self):
        self.assertFalse(self.report([113,215,219])['execution_allowed'])

    def test_unique_count(self):
        self.assertEqual(self.report()['stages'][2]['unique_required_unimplemented'],2)

    def test_backend_names_not_claimed_used(self):
        r=self.report();self.assertEqual(r['dynamic_api_string_counts'],{'gl':1,'vk':1})
        self.assertIn('does not prove',r['string_evidence_caveat'])

    def test_hardware_never_inferred(self):
        self.assertEqual(self.report()['stages'][5]['status'],'not_run')

    def test_profile_refuses_other_binary(self):
        with self.assertRaisesRegex(ValueError,'SHA-256 mismatch'):audit(b'other-build',[])
