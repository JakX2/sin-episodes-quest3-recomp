#!/usr/bin/env python3
"""Read-only owner-original C_StatsSystem::Init metadata audit.

Full owner-client SHA and complete function region are checked before
seven original rel32 calls and four registered string arguments.
Never emits private original executable code.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import CLIENT_SHA256,OriginalPE
INIT_VA=0x241FCAA0
INIT_SIZE=114
INIT_SHA="8cfdc286dfe3dafc355884aab427fb53eaa04c2db66e444b64fd1816fbc6b6d5"
CALLS=(
    (0x241FCAA7,0x241FF420),
    (0x241FCAB0,0x241FF8E0),
    (0x241FCABA,0x241FC5A0),
    (0x241FCACA,0x24204F30),
    (0x241FCADF,0x241CFF70),
    (0x241FCAF4,0x241CFF70),
    (0x241FCB09,0x241CFF70),
)
LITERALS={
    0x242AE008:"scripts/stats/stats_display_layout.txt",
    0x242AE114:"StaticStat",
    0x242AE108:"DynamicStat",
    0x2429B894:"ResetHUD",
}
CALLBACKS=(0x241FB7A0,0x241FB7E0,0x241FCA60)
class AuditError(ValueError):pass

def verify_layout(pe,expected_sha=INIT_SHA):
    if pe.base!=0x24000000:raise AuditError("Original image base mismatch")
    if hashlib.sha256(pe.read(INIT_VA,INIT_SIZE)).hexdigest()!=expected_sha:
        raise AuditError("Complete original StatsSystem Init SHA mismatch")
    for callsite,target in CALLS:
        expected=b"\xe8"+struct.pack("<i",target-callsite-5)
        if pe.read(callsite,5)!=expected:
            raise AuditError(f"Original StatsSystem Init call edge mismatch: {callsite:#x}")
    for va,literal in LITERALS.items():
        if pe.read(va,len(literal)+1)!=literal.encode("ascii")+b"\x00":
            raise AuditError(f"Original StatsSystem Init string mismatch: {va:#x}")
    # The original method does not branch around its seven calls and
    # terminates in 'mov al, 1; pop esi; ret'. The body hash pins control.
    if pe.read(0x241FCB0E,4)!=b"\xb0\x01\x5e\xc3":
        raise AuditError("Original StatsSystem Init return mismatch")
    return {
        "original_client_sha256":CLIENT_SHA256,
        "complete_method_va":hex(INIT_VA),
        "complete_method_size":INIT_SIZE,
        "complete_method_sha256":INIT_SHA,
        "verified_seven_calls":[{"callsite":hex(x),"callee":hex(y)}
            for x,y in CALLS],
        "original_layout_literal":LITERALS[0x242AE008],
        "registered_callback_names":[LITERALS[x] for x in
            (0x242AE114,0x242AE108,0x2429B894)],
        "registered_callback_original_vas":[hex(x) for x in CALLBACKS],
        "unconditional_true_return":True,
        "fully_recreated_helper_bodies":0,
        "limitations":["Original four setup helper implementations are opaque",
                       "Native callback binding bodies not reconstructed",
                       "No retail game-system Init execution proven"]
    }
def audit_original(path):
    blob=pathlib.Path(path).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=CLIENT_SHA256:
        raise AuditError("Owner-original client SHA mismatch")
    return verify_layout(OriginalPE(blob))
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("client")
    args=ap.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
