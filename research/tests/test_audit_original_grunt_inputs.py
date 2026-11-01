"""Synthetic original-free provenance fixtures. No Ritual DLL or SDK data here."""
import hashlib
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import audit_original_grunt_inputs as audit


def stub():
    block = [
        '@BaseClass base(BaseVehiclePassengerNPC, RappelNPC) = BaseGrunt',
        '[',
        ' spawnflags(Flags) =',
        ' [',
        '  65536 : "Start LookOff" : 0',
        '  16777216 : "Dont secondary fire" : 0',
        ' ]',
        ' GrenadeHeight(float) : "Grenade toss height" : 0',
        ' NumGrenades(choices) : "Number of Grenades" : "5" =',
        ' [',
        '  "999999" : "Unlimited"',
        ' ]'
    ]
    for name in audit.INPUTS:
        param = "bool" if name == "ThrowGrenades" else "void"
        block.append(' input '+name+'('+param+') : "fixture"')
    block.append(']')
    for cls, weapon in audit.DEFAULT_WEAPONS.items():
        block.extend([
            '@NPCClass base(BaseGrunt) studio("fixture.mdl") = '+cls+' : "Fixture"',
            '[',
            ' additionalequipment(choices) : "Weapons" : "'+weapon+'" =',
            ' [',
            '  "'+weapon+'" : "Fixture weapon"',
            ' ]',
            ']',
        ])
    fgd = ('\n'.join(block)+'\n').encode()
    strings = []
    for name in audit.INPUTS: strings.extend((name, 'Input'+name))
    for field, member in audit.FIELDS: strings.extend((field, member))
    strings.extend(audit.DEFAULT_WEAPONS)
    binary = b'\x00' + b'\x00'.join(x.encode('ascii') for x in strings) + b'\x00'
    return binary, fgd


def verify(binary, fgd):
    return audit.summarize(binary, fgd, hashlib.sha256(binary).hexdigest(),
                           hashlib.sha256(fgd).hexdigest())


class GruntProvenanceTests(unittest.TestCase):
    def test_complete_synthetic_provenance(self):
        dll, fgd = stub()
        result = verify(dll,fgd)
        self.assertTrue(result['original_server_dll_sha256_verified'])
        self.assertFalse(result['actual_ai_handlers_disassembled'])
        self.assertFalse(result['quest_hmd_tested'])

    def test_exact_byte_hash_fails_closed(self):
        dll, fgd = stub()
        with self.assertRaisesRegex(audit.EvidenceError,'SHA-256'):
            audit.summarize(dll,fgd)

    def test_missing_input_handler_pair(self):
        dll, fgd = stub()
        dll=dll.replace(b'InputLookOff\x00',b'InputNoLook\x00')
        with self.assertRaisesRegex(audit.EvidenceError,'DLL input registry pair'):
            verify(dll,fgd)

    def test_missing_fgd_declaration(self):
        dll, fgd = stub()
        fgd=fgd.replace(b'input LookOn(void)',b'input LookGone(void)')
        with self.assertRaisesRegex(audit.EvidenceError,'FGD input declaration'):
            verify(dll,fgd)

    def test_wrong_grenade_default(self):
        dll,fgd=stub()
        fgd=fgd.replace(b'"Number of Grenades" : "5"',b'"Number of Grenades" : "6"')
        with self.assertRaisesRegex(audit.EvidenceError,'default changed'):
            verify(dll,fgd)

    def test_wrong_equipment_default(self):
        dll,fgd=stub()
        fgd=fgd.replace(b'"Weapons" : "weapon_chaingun"',
                        b'"Weapons" : "weapon_npc_pistol"')
        with self.assertRaisesRegex(audit.EvidenceError,'equipment default'):
            verify(dll,fgd)

    def test_detached_handler_name_is_not_a_pair(self):
        dll,fgd=stub()
        dll=dll.replace(b'InputStartPatrolling\x00',b'XXXXXXXXXXXXXX\x00') + \
            b'\x00InputStartPatrolling\x00'
        with self.assertRaisesRegex(audit.EvidenceError,'DLL input registry pair'):
            verify(dll,fgd)

    def test_missing_dll_datamap_field(self):
        dll,fgd=stub()
        dll=dll.replace(b'm_iNumGrenades\x00',b'xxxxxxxxxxxxxx\x00')
        with self.assertRaisesRegex(audit.EvidenceError,'datamap field'):
            verify(dll,fgd)


if __name__ == "__main__":
    unittest.main()
