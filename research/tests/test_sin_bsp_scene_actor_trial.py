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


    def test_write_makes_missing_parent_and_verifies_exact_copy(self):
        with tempfile.TemporaryDirectory() as d:
            base=pathlib.Path(d)
            src=base/'original.bsp'
            dst=base/'isolated'/'maps'/'se1_finale01.bsp'
            original=fixture()
            src.write_bytes(original)
            self.assertEqual(trial.main([str(src),'--write',str(dst)]),0)
            self.assertTrue(dst.is_file())
            self.assertEqual(trial.main([str(src),'--verify',str(dst)]),0)
            self.assertEqual(src.read_bytes(),original)
            self.assertEqual(trial.main([str(src),'--write',str(dst)]),2)

            # One-byte corruption cannot be mistaken for a successful trial.
            damaged=bytearray(dst.read_bytes())
            damaged[-1] ^= 0x01
            dst.write_bytes(damaged)
            self.assertEqual(trial.main([str(src),'--verify',str(dst)]),2)
            self.assertEqual(src.read_bytes(),original)

    def test_verification_rejects_source_and_wrong_generic_mode(self):
        with tempfile.TemporaryDirectory() as d:
            base=pathlib.Path(d)
            src=base/'original.bsp'
            dst=base/'trial.bsp'
            src.write_bytes(fixture())
            self.assertEqual(trial.main([str(src),'--verify',str(src)]),2)
            self.assertEqual(trial.main([str(src),'--verify',str(dst)]),2)
            self.assertEqual(trial.main([str(src),'--write',str(dst),'--include-generic']),0)
            self.assertEqual(trial.main([str(src),'--verify',str(dst)]),2)
            self.assertEqual(trial.main([str(src),'--verify',str(dst),'--include-generic']),0)

    def test_named_generic_actor_changes_only_one_entity(self):
        source=fixture()
        changed,counts=trial.make_trial(source,generic_targets=['extra'])
        self.assertEqual(counts['npc_generic_actor:extra'],1)
        self.assertEqual(counts['npc_jessica'],1)
        _,_,_,_,body=trial.read_bsp(changed)
        self.assertIn(b'"classname" "generic_actor"\n"targetname" "extra"',body)
        self.assertIn(b'"classname" "npc_alyx"',body)
        self.assertEqual(source[1400:1404],changed[1400:1404])
        self.assertEqual(trial.read_bsp(source)[-1].count(b'generic_actor'),1)
        self.assertEqual(body.count(b'"classname" "generic_actor"'),1)

    def test_named_generic_actor_refuses_unsafe_targets(self):
        source=fixture()
        for target in ('missing','JC_Hospital','jess01','bad"quote'):
            with self.assertRaises(trial.BspError):
                trial.make_trial(source,generic_targets=[target])
        with self.assertRaises(trial.BspError):
            trial.make_trial(source,include_generic=True,generic_targets=['extra'])
        with self.assertRaises(trial.BspError):
            trial.make_trial(source,generic_targets=['extra','extra'])

    def test_targeted_trial_cli_write_and_verify(self):
        with tempfile.TemporaryDirectory() as folder:
            base=pathlib.Path(folder)
            source=base/'original.bsp'
            target=base/'heli_trial'/'se1_finale01.bsp'
            source.write_bytes(fixture())
            opts=['--generic-target','extra']
            self.assertEqual(trial.main([str(source),*opts,'--write',str(target)]),0)
            self.assertEqual(trial.main([str(source),*opts,'--verify',str(target)]),0)
            self.assertEqual(trial.main([str(source),'--verify',str(target)]),2)
            self.assertEqual(trial.main([str(source),*opts,'--write',str(target)]),2)


    def test_pickup_proxies_only_when_opted_in_and_with_dual_heli(self):
        original = fixture()
        pickup_entities = (
            b'{\n"classname" "weapon_magnum"\n"targetname" "pickup1"\n}\n'
            b'{\n"classname" "weapon_assault_rifle"\n}\n'
            b'{\n"classname" "weapon_scattergun"\n}\n'
            b'{\n"classname" "weapon_grenade"\n}\n'
            b'{\n"classname" "item_ammo_magnum"\n}\n'
            b'{\n"classname" "npc_druglab_grunt_elite"\n}\n'
            b'{\n"classname" "npc_generic_actor"\n"targetname" "end_hardcorps_heli1"\n}\n'
            b'{\n"classname" "npc_generic_actor"\n"targetname" "end_hardcorps_heli2"\n}\n'
        )
        old_ent = trial.read_bsp(original)[-1]
        new_ent = old_ent[:-1] + pickup_entities + b'\0'
        source = bytearray(original)
        entity_offset = len(source)
        struct.pack_into('<iii4s', source, 8, entity_offset, len(new_ent), 0, b'\0'*4)
        source.extend(new_ent)
        source = bytes(source)

        without, old_counts = trial.make_trial(source)
        self.assertNotIn('weapon_magnum', old_counts)
        self.assertIn(b'"classname" "weapon_magnum"', trial.read_bsp(without)[-1])

        targets = ['end_hardcorps_heli1', 'end_hardcorps_heli2']
        changed, counts = trial.make_trial(source, generic_targets=targets, pickup_proxies=True)
        actual = trial.read_bsp(changed)[-1]
        for old, replacement in trial.PICKUP_PROXIES.items():
            self.assertEqual(counts[old], 1)
            self.assertNotIn(('"classname" "' + old + '"').encode(), actual)
            self.assertIn(('"classname" "' + replacement + '"').encode(), actual)
        self.assertEqual(actual.count(b'"classname" "generic_actor"'), 2)
        self.assertIn(b'"classname" "npc_druglab_grunt_elite"', actual)
        self.assertIn(b'"targetname" "pickup1"', actual)
        self.assertEqual(changed[1400:1404], b'abcd')
        self.assertEqual(source[1400:1404], b'abcd')

        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            src = root / 'original.bsp'
            output = root / 'pickup_trial.bsp'
            src.write_bytes(source)
            options = ['--pickup-proxies', '--generic-target', targets[0],
                       '--generic-target', targets[1]]
            self.assertEqual(trial.main([str(src), *options, '--write', str(output)]), 0)
            self.assertEqual(trial.main([str(src), *options, '--verify', str(output)]), 0)
            self.assertEqual(trial.main([str(src), '--verify', str(output)]), 2)
            self.assertEqual(src.read_bytes(), source)


    def test_single_grunt_proxy_keeps_other_enemies_and_equips_ar2(self):
        original = fixture()
        block = (
            b'{\n"classname" "npc_druglab_grunt_assault_rifle"\n'
            b'"targetname" "roof_grunt_1_seq1"\n"squadname" "seq1"\n'
            b'"spawnflags" "516"\n}\n'
            b'{\n"classname" "npc_druglab_grunt_elite"\n'
            b'"targetname" "roof_grunt_3_seq1"\n}\n'
        )
        old_ent = trial.read_bsp(original)[-1]
        new_ent = old_ent[:-1] + block + b'\0'
        source = bytearray(original)
        struct.pack_into('<iii4s', source, 8, len(source), len(new_ent), 0, b'\0'*4)
        source.extend(new_ent)
        source = bytes(source)

        without, counts = trial.make_trial(source, pickup_proxies=True)
        self.assertNotIn('grunt:roof_grunt_1_seq1', counts)
        self.assertIn(b'"classname" "npc_druglab_grunt_assault_rifle"', trial.read_bsp(without)[-1])

        name = 'roof_grunt_1_seq1'
        out, counts = trial.make_trial(source, grunt_targets=[name], pickup_proxies=True)
        body = trial.read_bsp(out)[-1]
        self.assertEqual(counts['grunt:' + name], 1)
        self.assertIn(b'"classname" "npc_combine_s"', body)
        self.assertIn(b'"targetname" "roof_grunt_1_seq1"', body)
        self.assertIn(b'"squadname" "seq1"', body)
        self.assertIn(b'"spawnflags" "516"', body)
        self.assertIn(b'"additionalequipment" "weapon_ar2"', body)
        self.assertIn(b'"classname" "npc_druglab_grunt_elite"', body)
        self.assertEqual(source[1400:1404], out[1400:1404])
        self.assertEqual(source[8+16:1036], out[8+16:1036])

        with tempfile.TemporaryDirectory() as folder:
            base = pathlib.Path(folder)
            src = base / 'original.bsp'
            target = base / 'test' / 'grunt.bsp'
            src.write_bytes(source)
            opts = ['--grunt-target', name, '--pickup-proxies']
            self.assertEqual(trial.main([str(src), *opts, '--write', str(target)]), 0)
            self.assertEqual(trial.main([str(src), *opts, '--verify', str(target)]), 0)
            self.assertEqual(src.read_bytes(), source)

    def test_grunt_proxy_refuses_duplicates_and_bad_targets(self):
        original = fixture()
        def with_more(extra):
            old = trial.read_bsp(original)[-1]
            content = old[:-1] + extra + b'\0'
            buf = bytearray(original)
            struct.pack_into('<iii4s', buf, 8, len(buf), len(content), 0, b'\0'*4)
            buf.extend(content)
            return bytes(buf)
        grunt = (b'{\n"classname" "npc_druglab_grunt_assault_rifle"\n'
                 b'"targetname" "roof_grunt_1_seq1"\n}\n')
        with self.assertRaises(trial.BspError):
            trial.make_trial(with_more(grunt + grunt),
                             grunt_targets=['roof_grunt_1_seq1'])
        with self.assertRaises(trial.BspError):
            trial.make_trial(with_more(grunt), grunt_targets=['extra'])
        with self.assertRaises(trial.BspError):
            trial.make_trial(with_more(grunt),
                             grunt_targets=['roof_grunt_1_seq1', 'roof_grunt_1_seq1'])
        with self.assertRaises(trial.BspError):
            trial.make_trial(with_more(
                b'{\n"classname" "npc_generic_actor"\n'
                b'"targetname" "roof_grunt_1_seq1"\n}\n'
            ), grunt_targets=['roof_grunt_1_seq1'])
        with self.assertRaises(trial.BspError):
            trial.make_trial(with_more(
                grunt.replace(b'\n}', b'\n"additionalequipment" "weapon_sin_rifle"\n}')
            ), grunt_targets=['roof_grunt_1_seq1'])


    def test_explicit_equipment_override_does_not_touch_original(self):
        original = fixture()
        entity = (
            b'{\n"classname" "npc_druglab_grunt_assault_rifle"\n'
            b'"targetname" "roof_grunt_1_seq1"\n'
            b'"additionalequipment" "weapon_assault_rifle"\n'
            b'"spawnflags" "516"\n}\n'
            b'{\n"classname" "npc_druglab_grunt_elite"\n'
            b'"targetname" "untouched_elite"\n'
            b'"additionalequipment" "weapon_other"\n}\n'
        )
        modified_entity = trial.read_bsp(original)[-1][:-1] + entity + b'\0'
        source = bytearray(original)
        struct.pack_into('<iii4s', source, 8, len(source), len(modified_entity), 0, b'\0'*4)
        source.extend(modified_entity)
        source = bytes(source)
        with self.assertRaises(trial.BspError):
            trial.make_trial(source, grunt_targets=['roof_grunt_1_seq1'])
        copy, counts = trial.make_trial(
            source, grunt_targets=['roof_grunt_1_seq1'], grunt_equipment_override=True
        )
        body = trial.read_bsp(copy)[-1]
        self.assertEqual(counts['grunt:roof_grunt_1_seq1'], 1)
        self.assertIn(b'"additionalequipment" "weapon_ar2"', body)
        self.assertIn(b'"additionalequipment" "weapon_other"', body)
        self.assertNotIn(b'"additionalequipment" "weapon_assault_rifle"', body)
        self.assertIn(b'"additionalequipment" "weapon_assault_rifle"', trial.read_bsp(source)[-1])
        self.assertEqual(source[1400:1404], copy[1400:1404])
        with tempfile.TemporaryDirectory() as d:
            src, dst = pathlib.Path(d)/'source.bsp', pathlib.Path(d)/'trial.bsp'
            src.write_bytes(source)
            opts = ['--grunt-target', 'roof_grunt_1_seq1', '--grunt-equipment-override']
            self.assertEqual(trial.main([str(src), *opts, '--write', str(dst)]), 0)
            self.assertEqual(trial.main([str(src), *opts, '--verify', str(dst)]), 0)
            self.assertEqual(src.read_bytes(), source)


    def test_original_mobgrunt_model_is_opt_in_and_one_target_only(self):
        original = fixture()
        entities = (
            b'{\n"classname" "npc_druglab_grunt_assault_rifle"\n'
            b'"targetname" "roof_grunt_1_seq1"\n'
            b'"additionalequipment" "weapon_assault_rifle"\n'
            b'"spawnflags" "516"\n"sleepstate" "3"\n'
            b'"damagefilter" "grunt_unkillable"\n}\n'
            b'{\n"classname" "npc_druglab_grunt_elite"\n'
            b'"targetname" "roof_grunt_3_seq1"\n}\n'
        )
        body = trial.read_bsp(original)[-1][:-1] + entities + b'\0'
        source = bytearray(original)
        struct.pack_into('<iii4s', source, 8, len(source), len(body), 0, b'\0'*4)
        source.extend(body)
        source = bytes(source)
        options = dict(grunt_targets=['roof_grunt_1_seq1'],
                       grunt_equipment_override=True)
        base, _ = trial.make_trial(source, **options)
        self.assertNotIn(b'"model" "models/characters/MobGrunt/mobgrunt.mdl"',
                         trial.read_bsp(base)[-1])
        changed, counts = trial.make_trial(source, **options, grunt_original_model=True)
        rendered = trial.read_bsp(changed)[-1]
        self.assertEqual(counts['grunt:roof_grunt_1_seq1'], 1)
        self.assertEqual(rendered.count(b'"model" "models/characters/MobGrunt/mobgrunt.mdl"'), 1)
        self.assertIn(b'"additionalequipment" "weapon_ar2"', rendered)
        self.assertIn(b'"targetname" "roof_grunt_3_seq1"', rendered)
        self.assertIn(b'"classname" "npc_druglab_grunt_elite"', rendered)
        self.assertIn(b'"sleepstate" "3"', rendered)
        self.assertIn(b'"damagefilter" "grunt_unkillable"', rendered)
        self.assertEqual(source[1400:1404], changed[1400:1404])
        self.assertIn(b'"additionalequipment" "weapon_assault_rifle"',
                      trial.read_bsp(source)[-1])
        with tempfile.TemporaryDirectory() as folder:
            root = pathlib.Path(folder)
            src = root / 'original.bsp'
            dst = root / 'new' / 'one_grunt.bsp'
            src.write_bytes(source)
            flags = ['--grunt-target', 'roof_grunt_1_seq1',
                     '--grunt-equipment-override', '--grunt-original-model']
            self.assertEqual(trial.main([str(src), *flags, '--write', str(dst)]), 0)
            self.assertEqual(trial.main([str(src), *flags, '--verify', str(dst)]), 0)
            self.assertEqual(trial.main([str(src), '--grunt-original-model',
                                         '--write', str(root / 'bad.bsp')]), 2)
            self.assertFalse((root / 'bad.bsp').exists())
            self.assertEqual(src.read_bytes(), source)

    def test_original_model_refuses_other_classes_and_existing_model_conflict(self):
        base = fixture()
        def embedded(source):
            body = trial.read_bsp(base)[-1][:-1] + source + b'\0'
            data = bytearray(base)
            struct.pack_into('<iii4s', data, 8, len(data), len(body), 0, b'\0'*4)
            data.extend(body)
            return bytes(data)
        head = b'{\n"classname" "npc_druglab_grunt_assault_rifle"\n"targetname" "test"\n'
        with self.assertRaises(trial.BspError):
            trial.make_trial(
                embedded(head + b'"model" "models/combine_soldier.mdl"\n}\n'),
                grunt_targets=['test'], grunt_original_model=True
            )
        with self.assertRaises(trial.BspError):
            trial.make_trial(
                embedded(head + b'"model" "models/characters/MobGrunt/mobgrunt.mdl"\n'
                          b'"model" "models/characters/MobGrunt/mobgrunt.mdl"\n}\n'),
                grunt_targets=['test'], grunt_original_model=True
            )
        with self.assertRaises(trial.BspError):
            trial.make_trial(
                embedded(head.replace(b'npc_druglab_grunt_assault_rifle',
                                      b'npc_druglab_grunt_elite') + b'}\n'),
                grunt_targets=['test'], grunt_original_model=True
            )
        same, _ = trial.make_trial(
            embedded(head + b'"model" "models/characters/MobGrunt/mobgrunt.mdl"\n}\n'),
            grunt_targets=['test'], grunt_original_model=True
        )
        self.assertEqual(trial.read_bsp(same)[-1].count(
            b'"model" "models/characters/MobGrunt/mobgrunt.mdl"'), 1)

if __name__=='__main__': unittest.main()