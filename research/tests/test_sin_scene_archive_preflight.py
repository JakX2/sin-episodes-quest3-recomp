import importlib.util
import json
import pathlib
import tempfile
import unittest
import zipfile

MODULE=pathlib.Path(__file__).resolve().parents[1] / 'tools' / 'sin_scene_archive_preflight.py'
spec=importlib.util.spec_from_file_location('preflight',MODULE)
pf=importlib.util.module_from_spec(spec)
spec.loader.exec_module(pf)

class CheckScenes(unittest.TestCase):
    def test_alias_actor_token_and_missing_resolution(self):
        with tempfile.TemporaryDirectory() as d:
            z=pathlib.Path(d)/'scenes.zip';log=pathlib.Path(d)/'errors.log';out=pathlib.Path(d)/'report.json'
            with zipfile.ZipFile(z,'w') as archive:
                archive.writestr('scenes/se1_finale/teaser_jc_hospital.vcd',
                                  '// Choreo version 1\nactor "JC_Hospital"\n{\n}\n')
                archive.writestr('scenes/se1_highrise/jess_wakeupblade.vcd',
                                  'actor "!Target1"\n{\n}\n')
            log.write_text("Scene 'scenes\\se1_finale\\teaser_jc_hospital.vcd' missing!\n"
                           "Scene 'scenes\\se1_highrise\\jess_wakeupblade.vcd' missing!\n",encoding='utf-8')
            self.assertEqual(pf.main([str(z),'--log',str(log),'--json',str(out)]),0)
            result=json.loads(out.read_text())
            self.assertEqual(result['vcd_count'],2)
            self.assertEqual(result['log_scenes_found_in_archive'],2)
            self.assertEqual(result['example_actor_tokens']['scenes/se1_finale/teaser_jc_hospital.vcd'],['JC_Hospital'])
            self.assertEqual(result['example_actor_tokens']['scenes/se1_highrise/jess_wakeupblade.vcd'],['!Target1'])
            log.write_text("Scene 'scenes\\none.vcd' missing!\n",encoding='utf-8')
            self.assertEqual(pf.main([str(z),'--log',str(log)]),1)

if __name__=='__main__':unittest.main()