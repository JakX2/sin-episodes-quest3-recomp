"""Original-DLL-free mutation regressions for Phase 76 advisor evidence."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_dd_advisors import (
    SCORE_CODE_BRANCHES,SERVER_SITES,CLIENT_SITES,SERVER_CALLS,CLIENT_CALLS,
    verify_advisors,VerificationError
)

class FakePE:
    def __init__(self):self.data={}
    def put(self,va,raw):
        for offset,value in enumerate(raw):
            pos=va+offset
            if pos in self.data and self.data[pos]!=value:
                raise AssertionError("conflicting synthetic x86 fixture")
            self.data[pos]=value
    def at(self,va,count):
        return bytes(self.data.get(va+i,0xCC) for i in range(count))
    def corrupt(self,va):self.data[va]^=0x10

def fixtures():
    client,server=FakePE(),FakePE()
    server.put(0x222BA714,SCORE_CODE_BRANCHES)
    for pe,sites in ((server,SERVER_SITES),(client,CLIENT_SITES)):
        for va,raw in sites.items():pe.put(va,bytes.fromhex(raw))
    for pe,sites in ((server,SERVER_CALLS),(client,CLIENT_CALLS)):
        for va,target in sites.items():
            pe.put(va,b"\xe8"+struct.pack("<i",target-va-5))
    server.put(0x2245CB18,b"DDAdvisorScores\0")
    server.put(0x2245CB28,b"DDAdvisorChannels\0")
    server.put(0x2225A164,bytes.fromhex("6a10"))
    client.put(0x240A5600,bytes.fromhex("6a10"))
    server.put(0x22258F00,bytes.fromhex("6a20"))
    client.put(0x240A5631,bytes.fromhex("6a20"))
    return client,server

class AdvisorProtocolTests(unittest.TestCase):
    def test_complete_verified_field_contract(self):
        c,s=fixtures(); result=verify_advisors(c,s)
        data=result["DDAdvisorScores"]; channels=result["DDAdvisorChannels"]
        self.assertEqual(data["field_bits_without_framing"],288)
        self.assertEqual(channels["field_bits_without_framing"],768)
        self.assertTrue(data["server_last_two_groups_bypass_code_remap"])
        self.assertEqual(channels["client_four_float_vector_offsets"],
                         ["0x44","0x58","0x6C","0x80"])

    def test_score_condition_branch_mutations(self):
        for address in (0x222BA714,0x222BA717,0x222BA719,0x222BA71C,
                        0x222BA721,0x222BA72E,0x222BA73C,0x222BA743,
                        0x222BA747,0x222BA74E):
            with self.subTest(address=hex(address)):
                c,s=fixtures();s.corrupt(address)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

    def test_sign_and_float_width_mutations(self):
        for side,address in (("server",0x2225A164),("client",0x240A5600),
                             ("server",0x22258F00),("client",0x240A5631)):
            with self.subTest(address=hex(address)):
                c,s=fixtures();(s if side=="server" else c).corrupt(address+1)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

    def test_message_names_mutations(self):
        for address in (0x2245CB18,0x2245CB28):
            with self.subTest(address=hex(address)):
                c,s=fixtures();s.corrupt(address)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

    def test_server_sender_call_targets_mutations(self):
        for address in SERVER_CALLS:
            with self.subTest(address=hex(address)):
                c,s=fixtures();s.corrupt(address+1)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

    def test_client_receiver_call_targets_mutations(self):
        for address in CLIENT_CALLS:
            with self.subTest(address=hex(address)):
                c,s=fixtures();c.corrupt(address+1)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

    def test_channel_float_member_offsets_mutations(self):
        for address in (0x222BA7E5,0x222BA7F0,0x222BA7FB,0x222BA806):
            with self.subTest(address=hex(address)):
                c,s=fixtures();s.corrupt(address+2)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

    def test_six_group_loop_counts_mutations(self):
        for side,address in (("server",0x222BA75F),("server",0x222BA7DE),
                             ("client",0x241FAA7C),("client",0x241FA878)):
            with self.subTest(address=hex(address)):
                c,s=fixtures();(s if side=="server" else c).corrupt(address)
                with self.assertRaises(VerificationError):verify_advisors(c,s)

if __name__=="__main__":unittest.main()
