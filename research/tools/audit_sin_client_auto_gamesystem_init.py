#!/usr/bin/env python3
"""SHA-verified original 2006 client AutoGameSystem pending-vector Init audit.
Only reports public metadata, never emits or copies original code bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE, CLIENT_SHA256

IMAGE_BASE=0x24000000
REGIONS={
    "direct_register":(0x24156840,23,"e48841e7e14a9565ac425416c3fc3230615c8246fa603be65d7149500396292e"),
    "merge_and_init":(0x24156860,277,"b4f5ca8aefcb1248cdd7a62edf042a3f7fd5a04ac782fde277d13f335067ccb7"),
    "pending_constructor":(0x24156980,70,"5e6a0f107b7cf1f04f3331c1ca790683a42b104fd276dbc6e522c2b69483206b"),
    "client_init_callsite":(0x240FE4B3,13,"8a27594238d07a0193dafefa3a374f6bf92a3a5a7704c1580b933790d1d855c6")
}
ANCHORS={
    0x24156860:bytes.fromhex("a18c6c3624"), # pending head
    0x24156840:bytes.fromhex("8b0db4bf2f24"), # vector count
    0x24156890:bytes.fromhex("393c82"), # pending item equal registered
    0x24156935:bytes.fromhex("c7058c6c362400000000"), # clear head
    0x2415693F:bytes.fromhex("c605846c362401"), # mark started
    0x24156950:bytes.fromhex("a1a8bf2f24"), # vector backing data
    0x2415695A:bytes.fromhex("ff12"), # slot 0 virtual Init
    0x2415695C:bytes.fromhex("84c0"), # failure if bool zero
    0x2415696B:bytes.fromhex("5fb0015ec3"), # success
    0x24156970:bytes.fromhex("5f32c05ec3"), # failure
    0x24156980:bytes.fromhex("51803d846c362400"), # pending ctor once bit
    0x241569B8:bytes.fromhex("895604"), # node->next
    0x241569BB:bytes.fromhex("89358c6c3624") # pending head
}
DIRECT_CALLS=(
    0x240FE3FC,0x240FE407,0x240FE420,0x240FE42B,
    0x240FE436,0x240FE441,0x240FE44D,0x240FE457,
    0x240FE461,0x240FE46B,0x240FE476,0x240FE481
)
REGISTER_TARGET=0x24156840
CALL_VA=0x240FE4B3
NATIVE_MESSAGE_VA=0x2429EED0
NATIVE_MESSAGE=b"AutoGameSystem already added to game system list!!!\n\x00"
class AuditError(ValueError):pass

def verify_native(pe,regions=REGIONS):
    if pe.base!=IMAGE_BASE:raise AuditError("original client image base mismatch")
    for name,(va,length,sha) in regions.items():
        if hashlib.sha256(pe.read(va,length)).hexdigest()!=sha:
            raise AuditError("original method SHA mismatch: "+name)
    for addr,expected in ANCHORS.items():
        if pe.read(addr,len(expected))!=expected:
            raise AuditError("original instruction witness mismatch at %#x"%addr)
    if pe.read(NATIVE_MESSAGE_VA,len(NATIVE_MESSAGE))!=NATIVE_MESSAGE:
        raise AuditError("original duplicate-game-system warning mismatch")
    for callsite in DIRECT_CALLS:
        expected=b"\xe8"+struct.pack("<i",REGISTER_TARGET-callsite-5)
        if pe.read(callsite,5)!=expected:
            raise AuditError("original twelve direct system registration calls mismatch at %#x"%callsite)
    target=REGIONS["merge_and_init"][0]
    if pe.read(CALL_VA,5)!=b"\xe8"+struct.pack("<i",target-CALL_VA-5):
        raise AuditError("original client Init call to registry mismatch")
    return {
        "original_client_sha256":CLIENT_SHA256,
        "complete_function_or_callsite_regions":len(regions),
        "native_opcode_witnesses":len(ANCHORS),
        "client_init_registration_gate_va":hex(CALL_VA),
        "direct_registration_calls":len(DIRECT_CALLS),
        "direct_registration_callsite_vas":[hex(x) for x in DIRECT_CALLS],
        "merge_and_init_va":hex(target),
        "vector_data_global_va":"0x242FBFA8",
        "vector_count_global_va":"0x242FBFB4",
        "pending_list_global_va":"0x24366C8C",
        "started_flag_va":"0x24366C84",
        "pending_node_next_offset_x86":4,
        "callback_vtable_slot":0,
        "duplicate_behavior":"skip-and-warn-for-pending-only",
        "first_false_init_aborts":True,
        "registered_count_checked_live":True,
        "limits":["no original object constructors or engine class methods ported",
                  "no successful VClient011 Init or game boot"]
    }

def audit_original(path):
    blob=pathlib.Path(path).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=CLIENT_SHA256:
        raise AuditError("original client.dll full SHA256 mismatch")
    return verify_native(OriginalPE(blob))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("client")
    args=ap.parse_args()
    print(json.dumps(audit_original(args.client),sort_keys=True,indent=2))
if __name__=="__main__":main()
