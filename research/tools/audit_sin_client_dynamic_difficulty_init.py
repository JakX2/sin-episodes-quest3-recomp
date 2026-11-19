#!/usr/bin/env python3
"""Original SiN 2006 C_DynamicDifficulty Init complete-method audit.

Authenticates the original 234-byte body and all eleven ordered
named-callback registry calls, without exporting any executable bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE,CLIENT_SHA256
class AuditError(ValueError):pass
VA=0x241FAC90
LENGTH=234
SHA="e8d18d75c634b4e3a479db3d9e63b0af21ad666a95730183fbfd7c304b01b10e"
REGISTRY_GLOBAL=0x243010A8
REGISTRAR=0x241CFF70
# Full original call/registry binding evidence, no callback bodies.
BINDINGS=(
 ("GamePlayVarValue",0x242ADFF4,0x241F9960),
 ("Progression",0x242ADFE8,0x241F9970),
 ("DDInfo",0x242ADFE0,0x241F9F80),
 ("DDAdvisorChannels",0x242ADFCC,0x241FAC40),
 ("DDAdvisorScores",0x242ADFBC,0x241FAC50),
 ("DDDecisionMade",0x242ADFAC,0x241FAC60),
 ("DDDecisionsPerf",0x242ADF9C,0x241F9F90),
 ("DDSuccessModifiers",0x242ADF88,0x241F9990),
 ("ArenaMode",0x242ADF7C,0x241F9FA0),
 ("NewGameInfo",0x242ADF70,0x241F9FB0),
 ("Cheating",0x242ADF64,0x241F9FC0)
)
def verify_native(pe,body_sha=SHA):
    if pe.base!=0x24000000:
        raise AuditError("original PE image base mismatch")
    if hashlib.sha256(pe.read(VA,LENGTH)).hexdigest()!=body_sha:
        raise AuditError("original complete DynamicDifficulty Init SHA mismatch")
    for i,(name,nameva,callback) in enumerate(BINDINGS):
        entry=VA+i*0x15
        if pe.read(entry,6)!=b"\x8b\x0d"+struct.pack("<I",REGISTRY_GLOBAL):
            raise AuditError("original registry global load differs")
        if pe.read(entry+6,5)!=b"\x68"+struct.pack("<I",callback):
            raise AuditError("original callback address differs")
        if pe.read(entry+11,5)!=b"\x68"+struct.pack("<I",nameva):
            raise AuditError("original property name pointer differs")
        callsite=entry+16
        if pe.read(callsite,5)!=b"\xe8"+struct.pack("<i",REGISTRAR-callsite-5):
            raise AuditError("original callback registration call edge differs")
        if pe.read(nameva,len(name)+1)!=name.encode()+b"\x00":
            raise AuditError("original callback registry key text differs")
    if pe.read(VA+0xE7,3)!=b"\xb0\x01\xc3":
        raise AuditError("original DynamicDifficulty Init return not unconditional true")
    return {
        "original_client_sha256":CLIENT_SHA256,
        "init_va":hex(VA),"full_function_bytes":LENGTH,
        "original_sha256":SHA,
        "registry_global":hex(REGISTRY_GLOBAL),
        "registrar_va":hex(REGISTRAR),
        "bindings":[{"name":name,"name_va":hex(nameva),
            "callback_va":hex(callback),"callsite_va":hex(VA+i*0x15+16)}
            for i,(name,nameva,callback) in enumerate(BINDINGS)],
        "original_function_invoked":False,
        "limits":["callback target bodies not reimplemented",
                  "original registry symbol table not ported",
                  "no full VClient011 Init return or game boot"]
    }
def audit_original(path):
    blob=pathlib.Path(path).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=CLIENT_SHA256:
        raise AuditError("original client.dll full SHA256 mismatch")
    return verify_native(OriginalPE(blob))
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client")
    args=parser.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
