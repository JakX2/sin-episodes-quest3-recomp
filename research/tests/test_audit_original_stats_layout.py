"""Self-authored fixtures only: never include original SiN source scripts."""
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_original_stats_layout import inspect, LayoutError, parse_defs, graph_stats
D = '''"StatDefinitions" {
 "stat" { "name" "STAT_COUNT" "short_text" "#COUNT_SHORT" "long_text" "#COUNT_LONG" }
 "stat" { "name" "STAT_RATIO" "short_text" "#RATIO_SHORT" "long_text" "#RATIO_LONG"
          "percentage_of" "STAT_COUNT" }
}'''
P = '''"=========================="
"=========================="
"#TITLE" "tab"
"STAT_COUNT" "value"
"STAT_RATIO" "percent_of"
'''
G = '''"GraphPages" { "graph" { "name" "#SYNTHETIC" "maxvalue" "STAT_COUNT"
 "format" "percent" "stats" { "name" "STAT_RATIO" "name" "STAT_COUNT" } } }'''
class LayoutTests(unittest.TestCase):
    def test_fixture(self):
        result=inspect(D,P,G)
        self.assertEqual(result['definitions'],2)
        self.assertEqual(result['percentage_relationships'],1)
        self.assertEqual(result['display']['stat_rows'],2)
        self.assertEqual(result['graphs']['graph_stat_rows'],2)
        self.assertFalse(result['actual_difficulty_formula_verified'])
    def test_duplicate_stat(self):
        with self.assertRaisesRegex(LayoutError,'duplicate'):
            parse_defs(D.replace('"name" "STAT_RATIO"','"name" "STAT_COUNT"'))
    def test_unknown_percent_reference(self):
        with self.assertRaisesRegex(LayoutError,'unresolved'):
            parse_defs(D.replace('"percentage_of" "STAT_COUNT"',
                                 '"percentage_of" "STAT_OTHER"'))
    def test_no_graph(self):
        with self.assertRaises(LayoutError):
            graph_stats('"GraphPages" {}',parse_defs(D)[0])
    def test_bad_graph_reference(self):
        with self.assertRaisesRegex(LayoutError,'unknown graph stat'):
            graph_stats(G.replace('"name" "STAT_RATIO"','"name" "STAT_NOPE"'),
                        parse_defs(D)[0])
    def test_unknown_graph_maxvalue(self):
        with self.assertRaisesRegex(LayoutError,'unknown graph maxvalue'):
            graph_stats(G.replace('"maxvalue" "STAT_COUNT"',
                                  '"maxvalue" "STAT_UNKNOWN"'),parse_defs(D)[0])
    def test_bad_graph_format(self):
        with self.assertRaises(LayoutError):
            graph_stats(G.replace('"format" "percent"','"format" "bogus"'),
                        parse_defs(D)[0])
if __name__=='__main__':unittest.main()
