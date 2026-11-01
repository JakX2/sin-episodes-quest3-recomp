"""No original map or SDK assets used in these synthetic VMF tests."""
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_sin_sdk_spawn_placement import summary, SDKFormatError


def ent(kind, **fields):
    return ('entity\n{\n\t"classname" "'+kind+'"\n'+''.join(
        '\t"'+key+'" "'+str(value)+'"\n' for key,value in fields.items())+'}\n').encode()


class SpawnPlacementAuditTests(unittest.TestCase):
    def test_empty(self):
        self.assertEqual(summary(b'')['makers'],0)

    def test_defaults(self):
        d=summary(ent('npc_template_maker',TemplateName='secret'))
        self.assertEqual(d['templates_named'],1)
        self.assertEqual(d['radius'],{'256.0':1})
        self.assertEqual(d['visibility_criterion'],{'2':1})
        self.assertNotIn('secret',str(d))

    def test_flags_and_group_count_without_names(self):
        d=summary(ent('npc_template_maker',TemplateName='classified',DestinationGroup='private',
                      spawnflags=384,Radius=64,CriterionVisibility=1,CriterionDistance=0))
        self.assertEqual(d['makers_force_radius'],1)
        self.assertEqual(d['makers_visible_block'],1)
        self.assertEqual(d['makers_with_groups'],1)
        self.assertEqual(d['distance_criterion'],{'0':1})
        self.assertNotIn('private',str(d))
        self.assertNotIn('classified',str(d))

    def test_destination_fields(self):
        d=summary(ent('info_npc_spawn_destination',ReuseDelay=1.5,RenameNPC='hidden_npc'))
        self.assertEqual(d['destinations'],1)
        self.assertEqual(d['destination_reuse_delay_seconds'],{'1.5':1})
        self.assertEqual(d['destinations_with_rename'],1)
        self.assertNotIn('hidden_npc',str(d))

    def test_bad_visibility(self):
        with self.assertRaises(SDKFormatError):
            summary(ent('npc_template_maker',CriterionVisibility=5))

    def test_negative_radius(self):
        with self.assertRaises(SDKFormatError):
            summary(ent('npc_template_maker',Radius=-1))

    def test_nan_radius(self):
        with self.assertRaises(SDKFormatError):
            summary(ent('npc_template_maker',Radius='nan'))

    def test_bad_reuse_delay(self):
        for value in ('-1','nan','inf','oops'):
            with self.subTest(value=value),self.assertRaises(SDKFormatError):
                summary(ent('info_npc_spawn_destination',ReuseDelay=value))

    def test_no_secret_output_names(self):
        d=summary(ent('npc_template_maker',TemplateName='target_player_01',
                      targetname='SECRET_target',OnSpawnNPC='SECRET_command'))
        self.assertNotIn('SECRET',str(d))

    def test_mixed(self):
        data=(ent('npc_template_maker',TemplateName='private',DestinationGroup='grp')+
              ent('npc_template_maker',TemplateName='private2',Radius=0)+
              ent('info_npc_spawn_destination',ReuseDelay=1)+ent('info_target'))
        d=summary(data)
        self.assertEqual((d['makers'],d['destinations'],d['templates_named'],d['makers_with_groups']),
                         (2,1,2,1))


if __name__=='__main__':unittest.main()
