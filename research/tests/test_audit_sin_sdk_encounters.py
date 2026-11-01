"""Original-free VMF encounter-field tests; no original game files."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_sin_sdk_encounters import vmf_encounters, SDKFormatError


def entity(typ, **props):
    data = {'classname': typ, **props}
    return ('entity\n{\n' + ''.join(
        f'\t"{k}" "{v}"\n' for k, v in data.items()) + '}\n').encode()


class EncounterAuditTests(unittest.TestCase):
    def test_empty_and_unrelated(self):
        self.assertEqual(vmf_encounters(b'')['checkpoint_count'], 0)
        self.assertEqual(vmf_encounters(entity('light'))['maker_count'], 0)

    def test_checkpoint_defaults(self):
        r = vmf_encounters(entity('trigger_dynamic_difficulty'))
        self.assertEqual(r['checkpoint_count'], 1)
        self.assertEqual(r['difficulty_tier_counts'], {2: 1})
        self.assertEqual(r['estimated_time_seconds']['sum'], 0)

    def test_checkpoint_flags_and_timing(self):
        r = vmf_encounters(entity('trigger_dynamic_difficulty', EstimatedTime=123.5,
                                  EstimatedDifficulty=4, EndOfGame=1, spawnflags=1025))
        self.assertEqual(r['checkpoint_repeat_flag_count'], 1)
        self.assertEqual(r['checkpoint_endgame_count'], 1)
        self.assertEqual(r['difficulty_tier_counts'], {4: 1})
        self.assertEqual(r['estimated_time_seconds']['max'], 123.5)

    def test_maker_properties(self):
        r = vmf_encounters(entity('npc_template_maker', MaxLiveChildren=3,
                                  SpawnFrequency=2.5, StartDisabled=1))
        self.assertEqual(r['maker_count'], 1)
        self.assertEqual(r['maker_max_live_children_counts'], {3: 1})
        self.assertEqual(r['maker_start_disabled_counts'], {'1': 1})
        self.assertEqual(r['maker_spawn_frequency_counts'], {2.5: 1})

    def test_missing_maker_limit_is_reported(self):
        r = vmf_encounters(entity('npc_template_maker'))
        self.assertEqual(r['maker_no_live_limit_count'], 1)

    def test_tier_out_of_range_rejected(self):
        with self.assertRaisesRegex(SDKFormatError, 'range'):
            vmf_encounters(entity('trigger_dynamic_difficulty', EstimatedDifficulty=7))

    def test_negative_and_nonfinite_checkpoint_times_rejected(self):
        for value in ('-1', 'nan', 'inf'):
            with self.subTest(value=value), self.assertRaises(SDKFormatError):
                vmf_encounters(entity('trigger_dynamic_difficulty', EstimatedTime=value))

    def test_nonfinite_maker_rate_rejected(self):
        with self.assertRaisesRegex(SDKFormatError, 'nonfinite'):
            vmf_encounters(entity('npc_template_maker', SpawnFrequency='nan'))

    def test_malformed_top_level_entity_rejected(self):
        with self.assertRaises(SDKFormatError):
            vmf_encounters(b'entity\n{\n\t"classname" "trigger_dynamic_difficulty"\n')

    def test_summary_excludes_proprietary_targetnames(self):
        r = vmf_encounters(entity('npc_template_maker', targetname='private_original_name',
                                  TemplateName='private_original_template'))
        self.assertNotIn('private_original_name', str(r))
        self.assertNotIn('private_original_template', str(r))


if __name__ == '__main__':
    unittest.main()
