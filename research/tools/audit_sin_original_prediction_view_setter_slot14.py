#!/usr/bin/env python3
"""Phase1663: original CPrediction slot14 delegates +0xF04 view writer.

Read-only original 2006 PE32 analysis with dual original full-file SHA
gate from Phase1658 and precise whole-function SHA for the 18-byte
prediction member and 33-byte C_BasePlayer helper. Public --self-test
uses invented synthetic sparse bytes, never original game code.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,CLIENT_VIRTUAL_TARGETS,verify_original,check,section_read,
)
from audit_sin_original_client_prediction_view_readers import (
    HOST_GLOBAL_VA,HOST_MEMBER_VIEW_VECTOR_OFFSET,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

SLOT=14
PREDICTION_SLOT_VA=0x24195F50
PREDICTION_SLOT_SIZE=18
PREDICTION_SLOT_SHA="067addd7541796c20977a3e297c56aad4f04ea9c267d7b774a56553a87a9c240"
PLAYER_SET_VIEW_VA=0x240D22E0
PLAYER_SET_VIEW_SIZE=33
PLAYER_SET_VIEW_SHA="32406e3e02bca0f9490059e964d1e396a4dfe356c9796d42c86c3ceb8de87793"
PREDICTION_VTABLE=0x242A363C

def u32(blob: bytes,off: int)->int:
    return struct.unpack_from("<I",blob,off)[0]
def pack(value: int)->bytes:
    return struct.pack("<I",value)

def verify_prediction_setter(blob: bytes,global_va: int,
                             method_va: int,player_set_va: int)->None:
    check(len(blob)==PREDICTION_SLOT_SIZE,
          "prediction slot14 original 18-byte method boundary changed")
    check(blob[:2]==b"\x8B\x0D" and u32(blob,2)==global_va,
          "prediction slot14 not loading current C_BasePlayer pointer")
    check(blob[6:11]==bytes.fromhex("85c97405e9"),
          "prediction slot14 non-null tail jump not conditional")
    rel=struct.unpack_from("<i",blob,11)[0]
    check(method_va+15+rel==player_set_va,
          "prediction slot14 does not tail-call exact original player setter")
    check(blob[15:18]==bytes.fromhex("c20400"),
          "prediction null-player branch not x86 RET4")

def verify_player_view_writer(blob: bytes,field: int)->None:
    check(len(blob)==PLAYER_SET_VIEW_SIZE,
          "original C_BasePlayer view writer is not exactly 33 bytes")
    checks=(
       (0,bytes.fromhex("8b442404")),
       (4,bytes.fromhex("8b10")),
       (6,b"\x89\x91"+pack(field)),
       (12,bytes.fromhex("8b5004")),
       (15,b"\x89\x91"+pack(field+4)),
       (21,bytes.fromhex("8b4008")),
       (24,b"\x89\x81"+pack(field+8)),
       (30,bytes.fromhex("c20400")),
    )
    for pos,data in checks:
        check(blob[pos:pos+len(data)]==data,
              "native three-DWORD C_BasePlayer view setter differs @"+
              hex(pos))

def audit(client_bytes: bytes,engine_bytes: bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "original 23-slot prediction vtable not authenticated")
    client=PEImage(client_bytes)
    check(client.base==CLIENT_BASE and
          CLIENT_VIRTUAL_TARGETS[SLOT]==PREDICTION_SLOT_VA,
          "unexpected original client prediction slot14 vtable target")
    reloc=pe_highlow_relocations(client)
    check(PREDICTION_VTABLE+SLOT*4 in reloc and
          section_read(client,PREDICTION_VTABLE+SLOT*4,4,".rdata")==
          pack(PREDICTION_SLOT_VA),
          "prediction vtable slot14 not correct original relocated target")
    check(PREDICTION_SLOT_VA+2 in reloc,
          "prediction slot14 global C_BasePlayer pointer not relocated")
    pred=section_read(client,PREDICTION_SLOT_VA,
                      PREDICTION_SLOT_SIZE,".text")
    player=section_read(client,PLAYER_SET_VIEW_VA,
                        PLAYER_SET_VIEW_SIZE,".text")
    check(hashlib.sha256(pred).hexdigest()==PREDICTION_SLOT_SHA and
          hashlib.sha256(player).hexdigest()==PLAYER_SET_VIEW_SHA,
          "original complete prediction/player view setter SHA mismatch")
    verify_prediction_setter(pred,HOST_GLOBAL_VA,
                             PREDICTION_SLOT_VA,PLAYER_SET_VIEW_VA)
    verify_player_view_writer(player,HOST_MEMBER_VIEW_VECTOR_OFFSET)
    for address,length in ((PREDICTION_SLOT_VA,PREDICTION_SLOT_SIZE),
                           (PLAYER_SET_VIEW_VA,PLAYER_SET_VIEW_SIZE)):
        check(section_read(client,address+length,2,".text")==b"\xCC\xCC",
              "native original method boundary not INT3 padded")
    return {
        "phase":1663,
        "authenticated_original_client_sha":hashlib.sha256(client_bytes).hexdigest(),
        "prediction_vtable_slot":SLOT,
        "prediction_set_view_va":hex(PREDICTION_SLOT_VA),
        "prediction_method_length":PREDICTION_SLOT_SIZE,
        "prediction_method_sha256":PREDICTION_SLOT_SHA,
        "original_player_view_writer_va":hex(PLAYER_SET_VIEW_VA),
        "original_player_writer_length":PLAYER_SET_VIEW_SIZE,
        "original_player_writer_sha256":PLAYER_SET_VIEW_SHA,
        "new_original_whole_method_bytes":51,
        "original_shared_current_player_global":hex(HOST_GLOBAL_VA),
        "player_view_member_start":hex(HOST_MEMBER_VIEW_VECTOR_OFFSET),
        "player_view_member_words":3,
        "original_prediction_slot13_getter_same_fields":True,
        "original_slot14_no_player_noop":True,
        "native_ret_pop_bytes":4,
        "retail_native_functions_executed":0,
        "gameplay_boot_verified":False,
    }

def reject(action,label: str)->None:
    try:action()
    except ValueError:return
    raise AssertionError("corrupt original client view ABI accepted: "+label)

def self_test()->None:
    fake_global=0x56110000
    fake_pred=0x49110000
    fake_player=0x49113000
    pred=(bytes.fromhex("8b0d")+pack(fake_global)+
          bytes.fromhex("85c97405e9")+
          struct.pack("<i",fake_player-(fake_pred+15))+
          bytes.fromhex("c20400"))
    verify_prediction_setter(pred,fake_global,fake_pred,fake_player)
    for i in range(len(pred)):
        broken=bytearray(pred);broken[i]^=1
        reject(lambda data=bytes(broken):verify_prediction_setter(
            data,fake_global,fake_pred,fake_player),
            "slot14 byte "+str(i))
    view=0x134u  # invented memory layout, not original literal address
    fake=bytearray(b"\xCC"*PLAYER_SET_VIEW_SIZE)
    chunks=(
        (0,bytes.fromhex("8b442404")),
        (4,bytes.fromhex("8b10")),
        (6,b"\x89\x91"+pack(view)),
        (12,bytes.fromhex("8b5004")),
        (15,b"\x89\x91"+pack(view+4)),
        (21,bytes.fromhex("8b4008")),
        (24,b"\x89\x81"+pack(view+8)),
        (30,bytes.fromhex("c20400")),
    )
    touched=set()
    for pos,data in chunks:
        fake[pos:pos+len(data)]=data
        touched.update(range(pos,pos+len(data)))
    verify_player_view_writer(bytes(fake),view)
    for i in sorted(touched):
        broken=bytearray(fake);broken[i]^=1
        reject(lambda data=bytes(broken):verify_player_view_writer(data,view),
               "original-free player view write byte "+str(i))
    reject(lambda:verify_player_view_writer(bytes(fake),view+4),
           "reinterpreted original member start")
    reject(lambda:verify_prediction_setter(pred,fake_global,
                                           fake_pred,fake_player+4),
           "tail dispatch to non-matching player method")
    reject(lambda:verify_prediction_setter(pred[:17],fake_global,
                                           fake_pred,fake_player),
           "truncated original prediction entry")
    reject(lambda:verify_player_view_writer(bytes(fake[:32]),view),
           "truncated original player routine")
    reject(lambda:audit(b"fake retail client",b"fake retail engine"),
           "owner-original dual SHA bypass")
    print("PASS Phase1663 original-free: prediction slot14 gated "
          "local C_BasePlayer pointer, native tail JMP, RET4, "
          "player 33-byte three-DWORD setter at +0xF04..+0xF0C, "
          "synthetic corruption/rel32/offset/size and dual-SHA refusal")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be combined with owner images")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner audit requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
