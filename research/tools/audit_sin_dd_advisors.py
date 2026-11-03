#!/usr/bin/env python3
"""Verify original 2006 SiN advisor scores/channels x86 instructions.

Exact SHA-256 fingerprints, original PE32 code and paired sender/receiver
call edges required. No original DLL bytes are published. This is not a
complete Source usermessage codec or Android gameplay implementation.
"""
import argparse
import json
import struct
from pathlib import Path

from audit_sin_ddinfo import audit as prior_ddinfo_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

# Original server x86 score-code conditional chain at 0x222BA714..0x222BA750.
# Includes index 4/5 bypass and comparisons against constant 2.
SCORE_CODE_BRANCHES = bytes.fromhex(
    "83fe04743583fe0574308b434083f8027d0883f9027c0a83f8027e1e"
    "83f9027e1983f8027d073bc17e0983f8027e0b3bc17c07b802000000"
    "eb028bc1"
)

SERVER_SITES = {
    0x222BA6A0: "a120215922",
    0x222BA6A8: "83782c00",
    0x222BA6EE: "6818cb4522",  # DDAdvisorScores message name
    0x222BA6FC: "33f6",        # initialize index
    0x222BA6FE: "8d7b48",      # pointer array this+0x48
    0x222BA703: "8b421c",      # per-advisor float at +0x1C
    0x222BA70E: "8b4920",      # per-advisor code at +0x20
    0x222BA756: "83c601",      # next index
    0x222BA75C: "83c704",      # pointer array stride 4
    0x222BA75F: "83fe06",      # six entries
    0x222BA762: "7c9d",
    0x222BA780: "a120215922",
    0x222BA788: "83782c00",
    0x222BA7CD: "6828cb4522", # DDAdvisorChannels name
    0x222BA7D8: "8d7748",     # pointer array this+0x48
    0x222BA7DE: "bf06000000",
    0x222BA7E5: "8b4224",     # four float32 members
    0x222BA7F0: "8b5128",
    0x222BA7FB: "8b482c",
    0x222BA806: "8b4230",
    0x222BA812: "83c604",
    0x222BA815: "83ef01",
    0x222BA818: "75c9",
}
CLIENT_SITES = {
    0x241FAA6A: "8d7130",      # score float vector at +0x30
    0x241FAA72: "c7460c00000000", # reset length
    0x241FAA79: "8d6910",      # integer slots at +0x10
    0x241FAA7C: "c744241006000000",
    0x241FAAEB: "894500",      # integer stored at this+0x10+4*i
    0x241FAAEE: "83c504",
    0x241FAAF1: "836c241001",
    0x241FAAF6: "758c",
    0x241FAAFF: "c680b800000000",
    0x241FA859: "8d7944",      # four client vectors
    0x241FA85C: "89570c",
    0x241FA85F: "8d6958",
    0x241FA862: "89550c",
    0x241FA865: "8d596c",
    0x241FA868: "8d8180000000",
    0x241FA86E: "89530c",
    0x241FA875: "89500c",
    0x241FA878: "c744241406000000",
    0x241FAA39: "836c241401",
    0x241FAA3E: "0f853cfeffff",
    0x241FAA4B: "c680b800000000",
}
SERVER_CALLS = {
    0x222BA6F4:0x221E86D0,  # begin scores
    0x222BA707:0x221E8830,  # float32
    0x222BA751:0x221E87A0,  # signed16
    0x222BA764:0x221E8720,  # end scores
    0x222BA7D3:0x221E86D0,  # begin channels
    0x222BA7E9:0x221E8830,  # 4 float32 writes/entry
    0x222BA7F4:0x221E8830,
    0x222BA7FF:0x221E8830,
    0x222BA80A:0x221E8830,
    0x222BA81A:0x221E8720,  # end channels
}
CLIENT_CALLS = {
    0x241FAA86:0x240A5630,  # score f32
    0x241FAAE6:0x240A5600,  # score signed16
    0x241FA884:0x240A5630,  # 4 channel floats
    0x241FA8E6:0x240A5630,
    0x241FA94A:0x240A5630,
    0x241FA9AC:0x240A5630,
}

def checked(pe, site, original):
    if pe.at(site,len(original)) != original:
        raise VerificationError("SiN advisor original x86 byte mismatch at %#x" % site)

def checked_call(pe, site, target):
    checked(pe, site, b"\xe8"+struct.pack("<i",target-site-5))

def verify_advisors(client_pe, server_pe):
    checked(server_pe,0x222BA714,SCORE_CODE_BRANCHES)
    for pe, mapping in ((server_pe,SERVER_SITES),(client_pe,CLIENT_SITES)):
        for site,hexbytes in mapping.items():
            checked(pe,site,bytes.fromhex(hexbytes))
    for pe, mapping in ((server_pe,SERVER_CALLS),(client_pe,CLIENT_CALLS)):
        for site,target in mapping.items():checked_call(pe,site,target)
    checked(server_pe,0x2245CB18,b"DDAdvisorScores\0")
    checked(server_pe,0x2245CB28,b"DDAdvisorChannels\0")
    for pe,site,bytestr in (
        (server_pe,0x2225A164,b"\x6a\x10"),
        (client_pe,0x240A5600,b"\x6a\x10"),
        (server_pe,0x22258F00,b"\x6a\x20"),
        (client_pe,0x240A5631,b"\x6a\x20"),
    ):checked(pe,site,bytestr)
    return {
        "DDAdvisorScores":{
            "server_writer_va":"0x222BA6A0",
            "client_reader_va":"0x241FAA60",
            "six_groups":[ "f32", "s16" ],
            "field_bits_without_framing":288,
            "advisor_float_member_offset":"0x1C",
            "advisor_code_member_offset":"0x20",
            "server_context_member_offset":"0x40",
            "server_last_two_groups_bypass_code_remap":True,
            "client_float_vector_offset":"0x30",
            "client_s16_decoded_as_i32_start_offset":"0x10",
            "code_remap_equivalence":"for index<4: replace with 2 if (state<2 and raw<2 and state<=raw) or (state>2 and raw>2 and state>=raw)",
        },
        "DDAdvisorChannels":{
            "server_writer_va":"0x222BA780",
            "client_reader_va":"0x241FA850",
            "six_groups":["f32","f32","f32","f32"],
            "field_bits_without_framing":768,
            "advisor_float_member_offsets":["0x24","0x28","0x2C","0x30"],
            "client_four_float_vector_offsets":["0x44","0x58","0x6C","0x80"],
        },
        "caveat":"Field operations are not Source message framing, advisor meaning, game difficulty scoring or live Quest 3 modules.",
    }

def audit(client,server):
    previous=prior_ddinfo_audit(client,server) # original SHA-256, prior protocol evidence
    previous["advisor_scores_channels"]=verify_advisors(PEImage(client),PEImage(server))
    return previous

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    a=p.parse_args()
    out=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(out["advisor_scores_channels"],indent=2))
if __name__=="__main__":main()
