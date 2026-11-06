"""Synthetic original goal-assault x86 fixtures only. No private binaries."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "goal_input", TOOLS / "audit_sin_goal_assault_begin_input.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class FakePE:
    def __init__(self, memory):
        self.memory = memory

    def at(self, va, size):
        for address, raw in self.memory.items():
            if address <= va and va + size <= address + len(raw):
                off = va - address
                return raw[off:off + size]
        raise audit.VerificationError(f"synthetic VA absent: 0x{va:08X}")


def call(source, destination):
    return b"\xe8" + struct.pack("<i", destination - source - 5)


def fixture():
    memory = {
        va: bytes.fromhex(signature)
        for va, signature, _ in audit.WITNESSES
    }
    memory[audit.GOAL_ACTOR_SETUP_VTABLE_SLOT_VA] = struct.pack(
        "<I", audit.GOAL_ACTOR_SETUP_VA)
    for callsite, target, _ in audit.CALL_EDGES:
        memory[callsite] = call(callsite, target)
    for name, desc, name_va, dtype, offset, flags, external_va, external, handler in audit.GOAL_DATAMAP:
        memory[desc] = struct.pack(
            "<13I", dtype, name_va, offset, 0, (flags << 16) | 1,
            external_va, 0, handler, 0, 0 if dtype == 0 else 4, 0, 0, 0)
        memory[name_va] = name.encode("ascii") + b"\0"
        memory[external_va] = external.encode("ascii") + b"\0"
    return memory


SDK = b'''@PointClass base(Targetname) = ai_goal_assault :
"AI Goal Assault"
[
    rallypoint(target_destination) : "Rally Point Set" : ""
    AssaultCue(choices) : "Assault Cue" : 1 =
    [
        1 : "Entity System Input"
        2 : "Gunfire"
        3 : "Don't wait for a cue."
    ]
    input Activate( void ) : "Begin the assault behavior"
    input Deactivate( void ) : "Cease the assault behavior"
    input BeginAssault( void ) : "Begin assault phase"
]'''


class OriginalGoalAssaultInputTests(unittest.TestCase):
    def test_actual_datamap_x86_input_and_goal_setup(self):
        found = audit.verify_original_x86(FakePE(fixture()))
        self.assertEqual(found["original_entity"], "ai_goal_assault")
        self.assertEqual(found["verified_instruction_witnesses"],
                         len(audit.WITNESSES))
        self.assertEqual(found["verified_direct_call_edges"],
                         len(audit.CALL_EDGES))
        mapped = {f["member"]: f for f in found["original_goal_datamap"]}
        self.assertEqual(mapped["InputBeginAssault"]["hammer_name"],
                         "BeginAssault")
        self.assertEqual(mapped["InputBeginAssault"]["handler_va"],
                         "0x220CFF30")
        self.assertEqual(mapped["m_RallyPoint"]["x86_offset"], "0x3C8")
        self.assertEqual(mapped["m_AssaultCue"]["x86_offset"], "0x3CC")
        self.assertEqual(
            found["BeginAssault_input"]["effect_if_resolved"][1],
            "write CAI_AssaultBehavior.m_ReceivedAssaultCue +0x20 = 1")
        self.assertEqual(
            found["MobGrunt_direct_Assault_input"]["cue_mode"], 3)

    def test_original_sdk_goal_contract(self):
        found = audit.validate_sdk_fgd(SDK)
        self.assertEqual(found["cue_modes"][1], "Entity System Input")
        self.assertEqual(found["input"], "BeginAssault(void)")
        with self.assertRaises(audit.VerificationError):
            audit.validate_sdk_fgd(
                SDK.replace(b"input BeginAssault", b"input StartAssault"))
        with self.assertRaises(audit.VerificationError):
            audit.validate_sdk_fgd(
                SDK.replace(b"2 : \"Gunfire\"", b"2 : \"No Gunfire\""))

    def test_wrong_datamap_input_handler_rejected(self):
        memory = fixture()
        row = next(x for x in audit.GOAL_DATAMAP
                   if x[0] == "InputBeginAssault")
        name, desc, name_va, dtype, offset, flags, ext_va, external, _ = row
        memory[desc] = struct.pack(
            "<13I", dtype, name_va, offset, 0, (flags << 16) | 1,
            ext_va, 0, 0x220CFF00, 0, 0, 0, 0, 0)
        with self.assertRaises(audit.VerificationError):
            audit.verify_original_x86(FakePE(memory))

    def test_wrong_goal_cue_mode_field_rejected(self):
        memory = fixture()
        memory[0x220CFF15] = b"\x8b\x8e\xc8\x03\x00\x00"
        with self.assertRaises(audit.VerificationError):
            audit.verify_original_x86(FakePE(memory))

    def test_wrong_received_cue_value_rejected(self):
        memory = fixture()
        memory[0x220CFFA0] = b"\xc7\x41\x20\x03\x00\x00\x00"
        with self.assertRaises(audit.VerificationError):
            audit.verify_original_x86(FakePE(memory))

    def test_wrong_npc_condition_number_rejected(self):
        memory = fixture()
        memory[0x220CFFAA] = b"\x6a\x33"
        with self.assertRaises(audit.VerificationError):
            audit.verify_original_x86(FakePE(memory))

    def test_wrong_actor_count_and_array_offsets_rejected(self):
        for va, changed in (
            (0x220CFF65, b"\x3b\xbe\xbc\x03\x00\x00"),
            (0x220CF152, b"\x8b\x8e\xb0\x03\x00\x00"),
        ):
            memory = fixture()
            memory[va] = changed
            with self.assertRaises(audit.VerificationError):
                audit.verify_original_x86(FakePE(memory))

    def test_wrong_actor_resolver_or_condition_call_rejected(self):
        for callsite, target in (
            (0x220CFF70, 0x220CF130),
            (0x220CFFAC, 0x220397C0),
        ):
            memory = fixture()
            memory[callsite] = call(callsite, target)
            with self.assertRaises(audit.VerificationError):
                audit.verify_original_x86(FakePE(memory))

    def test_wrong_goal_activation_hook_rejected(self):
        memory = fixture()
        memory[audit.GOAL_ACTOR_SETUP_VTABLE_SLOT_VA] = struct.pack(
            "<I", audit.BEGIN_ASSAULT_HANDLER_VA)
        with self.assertRaises(audit.VerificationError):
            audit.verify_original_x86(FakePE(memory))

    def test_reject_unpinned_original_binary(self):
        with self.assertRaises(audit.VerificationError):
            audit.audit(b"not the real original DLL")


if __name__ == "__main__":
    unittest.main()
