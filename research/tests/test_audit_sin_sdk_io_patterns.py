"""Original-free unit tests for private aggregate VMF scripting metrics."""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(ROOT))
import audit_sin_sdk_io_patterns as audit


def synthetic_entity(klass, targetname='', connections=()):
    lines = ['entity', '{', f'\t"classname" "{klass}"']
    if targetname:
        lines.append(f'\t"targetname" "{targetname}"')
    if connections:
        lines += ['\tconnections', '\t{']
        lines += [f'\t\t"{key}" "{value}"' for key, value in connections]
        lines.append('\t}')
    lines.append('}')
    return '\n'.join(lines) + '\n'


class PatternAuditTests(unittest.TestCase):
    def test_static_name_categories_and_stagger(self):
        scene = (
            synthetic_entity('logic_relay', 'relay', [
                ('OnTrigger', 'beam,Break,,0.1,-1'),
                ('OnTrigger', 'beam,Break,,0.15,1'),
                ('OnTrigger', 'dupe,Enable,,0,1'),
                ('OnTrigger', 'fx_*,StopSound,,0,1'),
                ('OnTrigger', '!activator,Notify,,0,-1'),
                ('OnTrigger', 'future,Enable,,0,-1'),
                ('OutColor', 'beam,Color,255 0 0,0,0'),
            ]) + synthetic_entity('func_breakable', 'beam') +
            synthetic_entity('ambient_generic', 'dupe') +
            synthetic_entity('ambient_generic', 'DUPE'))
        output = audit.audit_io_patterns(scene.encode())
        counts = output['output_counts']
        self.assertEqual(output['entity_count'], 4)
        self.assertEqual(counts['connections'], 7)
        self.assertEqual(counts['delayed'], 2)
        self.assertEqual(counts['immediate'], 5)
        self.assertEqual(counts['one_shot'], 3)
        self.assertEqual(counts['disabled'], 1)
        self.assertEqual(counts['unlimited'], 3)
        self.assertEqual(counts['unique_literal_targets'], 3)
        self.assertEqual(counts['duplicate_name_targets'], 1)
        self.assertEqual(counts['special_targets'], 1)
        self.assertEqual(counts['wildcard_or_dynamic_targets'], 1)
        self.assertEqual(counts['unresolved_literal_targets'], 1)
        self.assertEqual(output['non_on_prefixed_output_connections'], 1)
        self.assertNotIn('beam', str(output))

    def test_invalid_data_categorization(self):
        scene = synthetic_entity('logic_relay','relay', [
            ('OnTrigger','bad,Open,,nan,1'),
            ('OnTrigger','bad,Open,,-1,-1'),
            ('OnTrigger','bad,Open,,0,-2'),
            ('OnTrigger','bad,Open,0'),
            ('OnTrigger',',Open,,0,1'),
        ])
        c = audit.audit_io_patterns(scene.encode())['output_counts']
        self.assertEqual(c['connections'], 5)
        self.assertEqual(c['invalid_delay_or_fire_count'], 3)
        self.assertEqual(c['wrong_field_count'], 1)
        self.assertEqual(c['empty_target_or_input'], 1)

    def test_top_level_malformed_rejected(self):
        with self.assertRaises(audit.SDKFormatError):
            audit.audit_io_patterns(b'entity\n{\n')

    def test_delimiter_variant(self):
        scene = synthetic_entity('logic_relay','a', [('OnTrigger','!caller\x1bUse\x1bp,a,r\x1b0.25\x1b1')])
        result = audit.audit_io_patterns(scene.encode())['output_counts']
        self.assertEqual(result['delayed'], 1)
        self.assertEqual(result['special_targets'], 1)
        self.assertEqual(result['wrong_field_count'], 0)


if __name__ == '__main__':
    unittest.main()
