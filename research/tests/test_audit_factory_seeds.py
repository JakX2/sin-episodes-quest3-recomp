import csv
import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_factory_seeds import audit

class AuditTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name)
        for module in ('client.dll', 'server.dll'):
            with (self.path / f'{module}_factory_seeds.csv').open('w', newline='') as f:
                w = csv.writer(f); w.writerow(['entry','interface','source']); w.writerow(['1000','ITest001','reviewed_original_static_registration'])
            with (self.path / f'{module}_factory_seed_results.csv').open('w', newline='') as f:
                w = csv.writer(f); w.writerow(['entry','interface','outcome','result_function_entry']); w.writerow(['1000','ITest001','seeded_candidate','1000'])
    def test_success(self):
        self.assertEqual(audit(self.path,self.path)['modules']['client.dll']['outcomes']['seeded_candidate'],1)
    def test_missing_record(self):
        p=self.path/'server.dll_factory_seed_results.csv'; p.write_text('entry,interface,outcome,result_function_entry\n')
        with self.assertRaisesRegex(ValueError,'missing'): audit(self.path,self.path)
    def test_bad_outcome(self):
        p=self.path/'server.dll_factory_seed_results.csv'; p.write_text('entry,interface,outcome,result_function_entry\n1000,ITest001,verified,1000\n')
        with self.assertRaisesRegex(ValueError,'unknown'): audit(self.path,self.path)
    def test_unresolved_has_entry(self):
        p=self.path/'server.dll_factory_seed_results.csv'; p.write_text('entry,interface,outcome,result_function_entry\n1000,ITest001,non_executable,1000\n')
        with self.assertRaisesRegex(ValueError,'unresolved'): audit(self.path,self.path)
    def test_duplicate(self):
        p=self.path/'server.dll_factory_seed_results.csv'
        with p.open('a') as f: f.write('1000,ITest001,seeded_candidate,1000\n')
        with self.assertRaisesRegex(ValueError,'duplicated'): audit(self.path,self.path)
    def test_mismatched_recovered_entry(self):
        p=self.path/'client.dll_factory_seed_results.csv'
        p.write_text('entry,interface,outcome,result_function_entry\n1000,ITest001,seeded_candidate,1004\n')
        with self.assertRaisesRegex(ValueError,'does not match'): audit(self.path,self.path)
    def test_malformed_seed_address(self):
        p=self.path/'client.dll_factory_seeds.csv'
        p.write_text('entry,interface,source\nnot_hex,ITest001,reviewed_original_static_registration\n')
        with self.assertRaisesRegex(ValueError,'invalid PE32 address'): audit(self.path,self.path)
    def test_malformed_result_address(self):
        p=self.path/'client.dll_factory_seed_results.csv'
        p.write_text('entry,interface,outcome,result_function_entry\n1000,ITest001,seeded_candidate,1000garbage\n')
        with self.assertRaisesRegex(ValueError,'invalid PE32 address'): audit(self.path,self.path)
    def test_canonical_hex_formats(self):
        p=self.path/'client.dll_factory_seed_results.csv'
        p.write_text('entry,interface,outcome,result_function_entry\n0x00001000,ITest001,already_inferred,00001000\n')
        self.assertEqual(audit(self.path,self.path)['modules']['client.dll']['outcomes']['already_inferred'],1)
    def test_out_of_range_address(self):
        p=self.path/'client.dll_factory_seeds.csv'
        p.write_text('entry,interface,source\n100000000,ITest001,reviewed_original_static_registration\n')
        with self.assertRaisesRegex(ValueError,'invalid PE32 address'): audit(self.path,self.path)
if __name__=='__main__': unittest.main()
