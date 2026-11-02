"""Commercial-binary-free x86 fixtures for exact original SiN difficulty messages."""
from __future__ import annotations

import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_difficulty_protocol import (
    MESSAGES,HOOKS,OTHER_CLIENT_HOOKS,CLIENT_DELEGATIONS,ACTION_FIELDS,
    DECISION_SERVER_WRITES,DECISION_CLIENT_READS,SUCCESS_CALLS,
    SUCCESS_LOOP_TEST,REGISTRY_FUNCTION,HOOK_REGISTER,
    audit,verify_evidence,VerificationError,
)

def rel32(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

class FakePE:
    def __init__(self):
        self.memory={}
    def put(self,va:int,raw:bytes)->None:
        for idx,byte in enumerate(raw):
            at=va+idx
            if at in self.memory and self.memory[at]!=byte:
                raise ValueError(f"conflicting synthetic evidence at {at:#x}")
            self.memory[at]=byte
    def at(self,va:int,size:int)->bytes:
        return bytes(self.memory.get(va+i,0xCC) for i in range(size))
    def corrupt(self,va:int)->None:
        self.memory[va]=(self.memory[va]^0x10)

def fixtures():
    client,server=FakePE(),FakePE()
    for name,(cv,cs,sv,ss,size) in MESSAGES.items():
        for pe,site,name_va,side in (
            (client,cs,cv,"client"),(server,ss,sv,"server")
        ):
            pe.put(site-2,bytes([0x6A,size&0xFF]))
            pe.put(site,b"\x68"+struct.pack("<I",name_va))
            pe.put(site+5,rel32(site+5,REGISTRY_FUNCTION[side]))
            pe.put(name_va,name.encode()+b"\x00")
        if name in HOOKS:
            site,cb=HOOKS[name]
            client.put(site-5,b"\x68"+struct.pack("<I",cb))
            client.put(site+5,rel32(site+5,HOOK_REGISTER))
    for name,(va,site,cb) in OTHER_CLIENT_HOOKS.items():
        client.put(va,name.encode()+b"\x00")
        client.put(site-5,b"\x68"+struct.pack("<I",cb))
        client.put(site,b"\x68"+struct.pack("<I",va))
        client.put(site+5,rel32(site+5,HOOK_REGISTER))
    for site,target in CLIENT_DELEGATIONS.values():
        client.put(site,rel32(site,target))
    for site,_,target in DECISION_SERVER_WRITES:
        server.put(site,rel32(site,target))
    for site,_,target in DECISION_CLIENT_READS:
        client.put(site,rel32(site,target))
    for site,target in SUCCESS_CALLS["client"]:
        client.put(site,rel32(site,target))
    for site,target in SUCCESS_CALLS["server"]:
        server.put(site,rel32(site,target))
    for side,pe in (("client",client),("server",server)):
        pe.put(SUCCESS_LOOP_TEST[side],bytes.fromhex("83fe0a"))
    for site,value_va in ((0x241FAB26,0x242DE2F8),(0x241FAB37,0x24276F84)):
        client.put(site,b"\xD8\x1D"+struct.pack("<I",value_va))
    client.put(0x242DE2F8,struct.pack("<f",0.0))
    client.put(0x24276F84,struct.pack("<f",-1.0))
    for name,(read_va,read_bytes,fmt_push_va,fmt_va,fmt_text,_,_) in ACTION_FIELDS.items():
        server.put(read_va,read_bytes)
        server.put(fmt_push_va,b"\x68"+struct.pack("<I",fmt_va))
        server.put(fmt_va,fmt_text)
    for name,site in (
        ("DDDecisionMade",0x222B9775),
        ("DDSuccessModifiers",0x222BA9F6),
        ("DDInfo",0x222BA5FD),
        ("DDAdvisorScores",0x222BA6EE),
        ("DDAdvisorChannels",0x222BA7CD),
    ):
        server.put(site,b"\x68"+struct.pack("<I",MESSAGES[name][2]))
    return client,server

class DifficultyProtocolTests(unittest.TestCase):
    def setUp(self):
        self.client,self.server=fixtures()

    def test_all_nine_message_names_register_with_matching_sizes(self):
        result=verify_evidence(self.client,self.server)
        self.assertEqual(result["paired_message_count"],9)
        self.assertEqual(result["client_hooks_in_dynamic_difficulty_block"],8)
        self.assertEqual(result["paired_messages"][0]["declared_message_length"],6)
        self.assertEqual(result["paired_messages"][1]["declared_message_length"],4)
        self.assertEqual(len([x for x in result["paired_messages"]
                              if x["declared_message_length"]==-1]),7)
        self.assertIsNone(next(x for x in result["paired_messages"]
                          if x["name"]=="DDAdvisorsUpdate")["client_callback_va"])

    def test_decision_payload_has_identical_typed_sequence(self):
        result=verify_evidence(self.client,self.server)
        record=result["decision_record"]
        self.assertEqual(record["server_writer_sequence"],
                         record["ordinary_record_sequence"])
        self.assertEqual(record["ordinary_record_sequence"],
                         ["f32","i32","i32","i32","f32","f32","f32","f32"])
        self.assertEqual(record["special_first_float_sentinels"],[0.0,-1.0])

    def test_success_modifiers_has_twenty_floats_in_ten_pairs(self):
        success=verify_evidence(self.client,self.server)["success_modifiers"]
        self.assertEqual(success["pairs_per_message"]*success["values_per_pair"],20)

    def test_server_difficulty_action_member_names_and_offsets(self):
        result=verify_evidence(self.client,self.server)
        fields=result["server_dynamic_difficulty_action_fields"]
        self.assertEqual({
            key: value["original_server_field_offset"] for key,value in fields.items()
        },{"time_started":"0x2BC","value":"0x2C8",
           "decision":"0x2C4","advisor":"0x2C0"})
        self.assertEqual(fields["advisor"]["type"],"i32")
        self.assertEqual(fields["time_started"]["type"],"f32")

    def test_corrupted_original_action_debug_field_read_is_rejected(self):
        self.server.corrupt(ACTION_FIELDS["decision"][0])
        with self.assertRaisesRegex(VerificationError,"member read"):
            verify_evidence(self.client,self.server)

    def test_corrupted_server_message_size_is_detected(self):
        self.server.corrupt(MESSAGES["GamePlayVarValue"][3]-1)
        with self.assertRaisesRegex(VerificationError,"declared length"):
            verify_evidence(self.client,self.server)

    def test_corrupted_client_hook_target_is_detected(self):
        self.client.corrupt(HOOKS["DDInfo"][0]-4)
        with self.assertRaisesRegex(VerificationError,"push-imm32"):
            verify_evidence(self.client,self.server)

    def test_corrupted_decision_reader_is_detected(self):
        self.client.corrupt(DECISION_CLIENT_READS[5][0]+1)
        with self.assertRaisesRegex(VerificationError,"CALL target"):
            verify_evidence(self.client,self.server)

    def test_corrupted_decision_writer_is_detected(self):
        self.server.corrupt(DECISION_SERVER_WRITES[2][0]+1)
        with self.assertRaisesRegex(VerificationError,"CALL target"):
            verify_evidence(self.client,self.server)

    def test_corrupted_success_loop_is_detected(self):
        self.server.corrupt(SUCCESS_LOOP_TEST["server"]+2)
        with self.assertRaisesRegex(VerificationError,"ten-pair"):
            verify_evidence(self.client,self.server)

    def test_corrupt_special_float_marker_rejected(self):
        self.client.corrupt(0x24276F84)
        with self.assertRaisesRegex(VerificationError,"negative sentinel"):
            verify_evidence(self.client,self.server)

    def test_original_dll_hash_pinning(self):
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            audit(b"fake client DLL",b"fake server DLL")

if __name__=="__main__":
    unittest.main()
