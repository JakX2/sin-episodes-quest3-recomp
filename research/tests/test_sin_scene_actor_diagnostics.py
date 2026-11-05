"""Fixture-only tests for actual scene/console diagnostics; no game assets."""
import importlib.util
import io
import pathlib
import struct
import sys
import tempfile
import unittest
import zipfile
from contextlib import redirect_stdout

TOOLS=pathlib.Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
import sin_scene_actor_diagnostics as diag


def map_fixture():
    payload=('''{
"classname" "worldspawn"
}
{
"classname" "npc_jc"
"targetname" "JC_Hospital"
"model" "models/jc.mdl"
}
{
"classname" "npc_jessica"
"targetname" "Jessica"
}
{
"classname" "logic_choreographed_scene"
"targetname" "sceneA"
"SceneFile" "se1_finale/teaser_jc_hospital.vcd"
"target1" "JC_Hospital"
"OnCompletion" "end,Trigger,,0,-1"
"OnCompletion" "other,Trigger,,0,-1"
}
{
"classname" "logic_choreographed_scene"
"SceneFile" "scenes/se1_highrise/jess_wakeupblade.vcd"
"target1" "Jessica"
}
''').encode()+b'\0'
    header=bytearray(1036);header[:4]=b'VBSP'
    struct.pack_into('<i',header,4,20)
    struct.pack_into('<iii4s',header,8,1036,len(payload),0,b'\0'*4)
    return bytes(header)+payload


class DiagnosticsTest(unittest.TestCase):
    def test_entity_pairs_preserve_repeated_outputs(self):
        entities=diag.entities_from_bsp(map_fixture())
        self.assertEqual(len(entities),5)
        self.assertEqual([v for k,v in entities[3] if k=='OnCompletion'],
                         ['end,Trigger,,0,-1','other,Trigger,,0,-1'])
    def test_scene_tokens_direct_and_alias(self):
        with tempfile.TemporaryDirectory() as t:
            base=pathlib.Path(t)
            bsp=base/'test.bsp';bsp.write_bytes(map_fixture())
            scenes=base/'scenes.zip'
            with zipfile.ZipFile(scenes,'w') as z:
                z.writestr('scenes/se1_finale/teaser_jc_hospital.vcd',
                           'actor "JC_Hospital"\n')
                z.writestr('scenes/se1_highrise/jess_wakeupblade.vcd',
                           'actor "!Target1"\n')
            result=diag.preflight(bsp,scenes,[])
            self.assertEqual(result['source_actor_counts']['npc_jc'],1)
            self.assertEqual(result['source_scene_controller_count'],2)
            binding=result['scene_bindings']
            self.assertTrue(binding[0]['actors'][0]['targetname_candidate_present'])
            self.assertEqual(binding[1]['actors'][0]['aliased_targetnames'],['Jessica'])
            self.assertTrue(binding[1]['actors'][0]['alias_set_in_controller'])
            self.assertTrue(all(x['present_in_supplied_archive'] for x in binding))
    def test_unresolved_alias_not_falsely_claimed_resolved(self):
        with tempfile.TemporaryDirectory() as t:
            base=pathlib.Path(t);bsp=base/'map.bsp';bsp.write_bytes(map_fixture())
            scenes=base/'scenes.zip'
            with zipfile.ZipFile(scenes,'w') as z:
                z.writestr('scenes/se1_highrise/jess_wakeupblade.vcd','actor "!Target2"\n')
            result=diag.preflight(bsp,scenes,[])
            self.assertFalse(result['scene_bindings'][1]['actors'][0]['alias_set_in_controller'])
    def test_log_comparison_does_not_claim_animation(self):
        before='''Attempted to create unknown entity type npc_jessica!\nAttempted to create unknown entity type npc_jc!\nScene 'scenes\\abc.vcd' missing!\n'''
        after='''Attempted to create unknown entity type npc_druglab_grunt_elite!\nScene 'scenes\\abc.vcd' missing!\n'''
        r=diag.compare(before,after)
        self.assertEqual(r['actor_unknown_classnames']['npc_jc']['after'],0)
        self.assertEqual(r['actor_unknown_classnames']['npc_jessica']['before'],1)
        self.assertEqual(r['missing_scene_deltas'][0]['after'],1)
        self.assertTrue(any('not proof' in line for line in r['interpretation']))
    def test_output_refuses_overwrite_and_does_not_modify_inputs(self):
        with tempfile.TemporaryDirectory() as t:
            base=pathlib.Path(t);before=base/'before.txt';after=base/'after.txt';report=base/'r.json'
            before.write_text('Attempted to create unknown entity type npc_jc!\n')
            after.write_text('')
            with redirect_stdout(io.StringIO()):
                self.assertEqual(diag.main(['compare','--before',str(before),'--after',str(after),'--output',str(report)]),0)
                self.assertEqual(diag.main(['compare','--before',str(before),'--after',str(after),'--output',str(report)]),2)
            self.assertEqual(before.read_text(),'Attempted to create unknown entity type npc_jc!\n')
    def test_baseline_multi_log_sum(self):
        with tempfile.TemporaryDirectory() as t:
            base=pathlib.Path(t);a=base/'one.txt';b=base/'two.txt';out=base/'baseline.json'
            a.write_text('Attempted to create unknown entity type npc_jc!\n')
            b.write_text('Attempted to create unknown entity type npc_jc!\nAttempted to create unknown entity type npc_jessica!\n')
            with redirect_stdout(io.StringIO()):
                self.assertEqual(diag.main(['baseline','--log',str(a),'--log',str(b),'--output',str(out)]),0)
            import json
            report=json.loads(out.read_text())
            self.assertEqual(report['aggregate']['unknown_entity_attempts'],3)
            self.assertEqual(report['aggregate']['actor_attempts']['npc_jc'],2)
            self.assertEqual(report['aggregate']['unknown_classnames'],2)
            self.assertNotIn(str(base),out.read_text())
    def test_bad_bsp_is_refused(self):
        with self.assertRaises(ValueError):diag.entities_from_bsp(b'bad')

if __name__=='__main__':unittest.main()