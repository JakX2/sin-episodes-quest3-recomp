"""Synthetic map entity tests: no proprietary SiN BSP included."""
import importlib.util
from pathlib import Path
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
spec = importlib.util.spec_from_file_location(
    'sin_assault_graph', TOOLS / 'analyze_sin_assault_graph.py')
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def ent(*values):
    return list(values)


class OriginalAssaultGraphTests(unittest.TestCase):
    def test_original_rally_chain_and_logic_relay_input(self):
        entities = [
            ent(('classname', 'worldspawn')),
            ent(('classname', 'assault_rallypoint'), ('targetname', 'rally_seq1_a'),
                ('priority', '2'), ('assaultpoint', 'assault_a')),
            ent(('classname', 'assault_assaultpoint'), ('targetname', 'assault_a'),
                ('nextassaultpoint', 'assault_b')),
            ent(('classname', 'assault_assaultpoint'), ('targetname', 'assault_b'),
                ('assaulttimeout', '3.0')),
            ent(('classname', 'ai_goal_assault'), ('targetname', 'assault_goal'),
                ('actor', 'roof_grunt_2_seq1'), ('rallypoint', 'rally_seq1_*')),
            ent(('classname', 'npc_druglab_grunt_assault_rifle'),
                ('targetname', 'roof_grunt_2_seq1')),
            ent(('classname', 'logic_relay'), ('targetname', 'begin_roof_battle_seq1'),
                ('OnTrigger', 'roof_grunt_2_seq1,Assault,rally_seq1_a,0.05,1'),
                ('OnTrigger', 'roof_grunt_1_seq1,Wake,,0,1')),
        ]
        value = a.analyze_entities(entities)
        self.assertEqual(value['summary']['rally_points'], 1)
        self.assertEqual(value['summary']['assault_points'], 2)
        self.assertEqual(value['summary']['goals'], 1)
        self.assertEqual(value['summary']['directed_assault_inputs'], 1)
        self.assertEqual(value['rally_points'][0]['assault_point']['status'], 'ok')
        self.assertEqual(value['assault_points'][0]['next_assault_point']['status'], 'ok')
        self.assertEqual(value['assault_goals'][0]['matching_rallypoints'][0]['index'], 1)
        signal = value['original_assault_input_signals'][0]
        self.assertEqual(signal['recipient'], 'roof_grunt_2_seq1')
        self.assertEqual(signal['rallypoint_parameter']['status'], 'ok')
        self.assertEqual(signal['delay'], '0.05')

    def test_missing_wrong_class_and_ambiguous_rallypoint(self):
        entities = [
            ent(('classname', 'logic_relay'), ('targetname', 'relay'),
                ('OnTrigger', 'grunt,Assault,missing,0,-1'),
                ('OnTrigger', 'grunt,Assault,ap1,0,-1'),
                ('OnTrigger', 'grunt,Assault,rp,0,-1')),
            ent(('classname', 'assault_assaultpoint'), ('targetname', 'ap1')),
            ent(('classname', 'assault_rallypoint'), ('targetname', 'rp')),
            ent(('classname', 'assault_rallypoint'), ('targetname', 'rp')),
        ]
        value = a.analyze_entities(entities)
        self.assertEqual([p['rallypoint_parameter']['status']
                          for p in value['original_assault_input_signals']],
                         ['missing', 'wrong_class', 'ambiguous'])

    def test_original_begin_assault_targets_goal_not_individual_npc(self):
        entities = [
            ent(('classname', 'ai_goal_assault'), ('targetname', 'goal_a'),
                ('actor', 'roof_grunt_1_seq1'), ('rallypoint', 'rp_a'),
                ('AssaultCue', '1')),
            ent(('classname', 'npc_druglab_grunt_assault_rifle'),
                ('targetname', 'roof_grunt_1_seq1')),
            ent(('classname', 'assault_rallypoint'),
                ('targetname', 'rp_a')),
            ent(('classname', 'logic_relay'), ('targetname', 'battle_start'),
                ('OnTrigger', 'goal_a,BeginAssault,,0,1'),
                ('OnTrigger', 'roof_grunt_1_seq1,Assault,rp_a,0.2,1')),
        ]
        value = a.analyze_entities(entities)
        self.assertEqual(value['summary']['directed_begin_assault_inputs'], 1)
        self.assertEqual(value['summary']['directed_assault_inputs'], 1)
        signal = value['original_goal_begin_assault_signals'][0]
        self.assertEqual(signal['goal_recipient']['status'], 'ok')
        self.assertEqual(signal['goal_recipient']['matching_expected_class'][0]['index'], 0)
        self.assertEqual(signal['recipient'], 'goal_a')
        self.assertEqual(signal['parameter'], '')
        self.assertEqual(value['original_assault_input_signals'][0]
                         ['rallypoint_parameter']['status'], 'ok')

    def test_begin_assault_wrong_class_missing_ambiguous_and_dynamic(self):
        entities = [
            ent(('classname', 'logic_relay'),
                ('OnTrigger', 'grunt,BeginAssault,,0,-1'),
                ('OnTrigger', 'missing_goal,BeginAssault,,0,-1'),
                ('OnTrigger', 'dup_goal,BeginAssault,,0,-1'),
                ('OnTrigger', '!activator,BeginAssault,,0,-1')),
            ent(('classname', 'npc_druglab_grunt_assault_rifle'),
                ('targetname', 'grunt')),
            ent(('classname', 'ai_goal_assault'), ('targetname', 'dup_goal')),
            ent(('classname', 'ai_goal_assault'), ('targetname', 'dup_goal')),
        ]
        value = a.analyze_entities(entities)
        self.assertEqual(
            [row['goal_recipient']['status']
             for row in value['original_goal_begin_assault_signals']],
            ['wrong_class', 'missing', 'ambiguous', 'dynamic_target'])
        self.assertEqual(value['summary']['directed_begin_assault_inputs'], 4)
        self.assertEqual(value['summary']['point_or_input_link_status']['dynamic_target'], 1)

    def test_begin_assault_esc_separated_output(self):
        entities = [
            ent(('classname', 'ai_goal_assault'), ('targetname', 'goal_a')),
            ent(('classname', 'logic_relay'),
                ('OnTrigger', 'goal_a\x1bBeginAssault\x1b\x1b0\x1b1')),
        ]
        value = a.analyze_entities(entities)
        self.assertEqual(value['summary']['directed_begin_assault_inputs'], 1)
        self.assertEqual(value['original_goal_begin_assault_signals'][0]
                         ['goal_recipient']['status'], 'ok')

    def test_output_esc_separator_and_malformed_rejected(self):
        self.assertEqual(a.parse_output('x\x1bAssault\x1brally\x1b0\x1b1')['input'],
                         'Assault')
        self.assertIsNone(a.parse_output('x,Assault'))
        self.assertEqual(a.normalize('  RaLLy_1 '), 'rally_1')


if __name__ == '__main__':
    unittest.main()
