"""No proprietary BSPs are needed for synthetic patcher tests."""
import importlib.util
import pathlib
import struct
import tempfile
import unittest

MODULE=pathlib.Path(__file__).resolve().parents[1] / 'tools' / 'sin_bsp_scene_actor_trial.py'
spec=importlib.util.spec_from_file_location('trial',MODULE)
trial=importlib.util.module_from_spec(spec)
spec.loader.exec_module(trial)


def fixture():
    header=bytearray(1036)
    header[:4]=b'VBSP'
    struct.pack_into('<i',header,4,20)
    # A synthetic, non-entity lump that must remain byte-identical.
    struct.pack_into('<iii4s',header,24,1400,4,0,b'\0'*4)
    ent=(b'{\n"classname" "worldspawn"\n}\n'
         b'{\n"classname" "npc_jc"\n"targetname" "JC_Hospital"\n}\n'
         b'{\n"classname" "npc_jessica"\n"targetname" "jess01"\n}\n'
         b'{\n"classname" "npc_generic_actor"\n"targetname" "extra"\n}\n\0')
    struct.pack_into('<iii4s',header,8,1036,len(ent),0,b'\0'*4)
    out=header+ent
    out.extend(b'\0'*(1400-len(out)))
    out.extend(b'abcd')
    return bytes(out)

class TrialTest(unittest.TestCase):
    def test_patch_preserves_original_lumps_and_bindings(self):
        b=fixture()
        changed,counts=trial.make_trial(b)
        self.assertEqual(counts,{'npc_jc':1,'npc_jessica':1})
        _,off,size,_,ent=trial.read_bsp(changed)
        self.assertGreater(off,len(b)-1)
        self.assertIn(b'"classname" "npc_citizen"',ent)
        self.assertIn(b'"classname" "npc_alyx"',ent)
        self.assertIn(b'"classname" "npc_generic_actor"',ent)
        self.assertIn(b'"targetname" "JC_Hospital"',ent)
        self.assertEqual(changed[1400:1404],b'abcd')
        self.assertEqual(b[8+16:1036],changed[8+16:1036])
    def test_include_generic_is_opt_in(self):
        changed,counts=trial.make_trial(fixture(),True)
        self.assertEqual(counts['npc_generic_actor'],1)
        self.assertIn(b'"classname" "npc_citizen"',trial.read_bsp(changed)[-1])
    def test_refuse_missing_lump_and_bad_header(self):
        for b in (b'bad',b'VBSP'+b'\0'*1100):
            with self.assertRaises(trial.BspError):trial.read_bsp(b)
    def test_refuse_compressed_lump(self):
        b=bytearray(fixture())
        b[20:24]=b'LZMA'
        with self.assertRaises(trial.BspError):trial.make_trial(bytes(b))
    def test_report_and_write_are_explicit(self):
        with tempfile.TemporaryDirectory() as d:
            src=pathlib.Path(d)/'map.bsp';out=pathlib.Path(d)/'map_preview.bsp'
            src.write_bytes(fixture())
            self.assertEqual(trial.main([str(src),'--report']),0)
            self.assertFalse(out.exists())
            self.assertEqual(trial.main([str(src),'--write',str(out)]),0)
            self.assertTrue(out.exists())
            self.assertEqual(src.read_bytes(),fixture())
            self.assertEqual(trial.main([str(src),'--write',str(out)]),2)
            self.assertEqual(trial.main([str(src),'--write',str(src)]),2)

if __name__=='__main__': unittest.main()