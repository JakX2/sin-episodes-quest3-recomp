#!/usr/bin/env python3
"""Owner-original 2006 SiN server.dll Elite virtual+0x424 weapon tick audit.

Verifies the COMPLETE native method SHA256, primary vtable slot, exact two
calls to the original weapon updater, inherited tail jumps, and the adjacent
secondary Elite event-handler's return-to-gate edge. No native body is dumped.
CI --self-test uses invented instruction arrays only, not commercial DLLs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

from verify_gameplay_entity_registry import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256
)
TICK=0x223555F0
TICK_SIZE=113
TICK_SHA="daede98b8e9cdc123089f1ac3b9e284382dd5e82e062781dd4db0e8063ad6f2e"
VTABLE=0x2248DF04
SLOT_OFFSET=0x424
WEAPON_UPDATE=0x22315390
PARENT_TICK=0x2235DF10
HANDLE=0xFDC
SECONDARY_EVENT=0x223556A0
SECONDARY_EVENT_SIZE=232
SECONDARY_EVENT_SHA="66a3abbe73c87aaa6ea5f7c9c97547b99a4c2fe6e626c6bb1f77c1a8a8c60262"
SECONDARY_SLOT=0x44C
SECONDARY_GATE_CALL=0x22355772
SOURCE_GATE=0x22315320
WITNESSES=(
 (0x223555F0,"568bf1"),
 (0x223555F3,"8b86dc0f0000"),
 (0x223555F9,"83f8ff"),
 (0x223555FC,"745b"),
 (0x223555FE,"8b1520564f22"),
 (0x22355606,"81e1ff0f0000"),
 (0x2235560C,"c1e104"),
 (0x22355613,"c1e80c"),
 (0x22355616,"394104"),
 (0x2235561B,"8b09"),
 (0x2235561D,"85c9"),
 (0x2235561F,"7438"),
 (0x22355621,"8b86dc0f0000"),
 (0x22355627,"83f8ff"),
 (0x2235562A,"7426"),
 (0x2235562E,"81e1ff0f0000"),
 (0x22355634,"c1e104"),
 (0x2235563B,"c1e80c"),
 (0x2235563E,"394104"),
 (0x22355643,"8b09"),
 (0x2235564A,"8bce"),
 (0x22355652,"33c9"),
 (0x22355659,"8bce"),
 (0x22355661,"cc"),
)
def sha(reader,address,n,expected):
    data=reader(address,n)
    if len(data)!=n or hashlib.sha256(data).hexdigest()!=expected:
        raise VerificationError("complete original native method hash changed at "+
                                hex(address))
def witness(reader,address,expected):
    want=bytes.fromhex(expected)
    if reader(address,len(want))!=want:
        raise VerificationError("native instruction witness mismatch at "+hex(address))
def rel32(reader,at,opcode,target):
    value=reader(at,5)
    if len(value)!=5 or value[0]!=opcode or (
       at+5+struct.unpack_from("<i",value,1)[0]!=target):
        raise VerificationError("native call/jump edge mismatch at "+hex(at))
def ptr32(reader,at,target):
    raw=reader(at,4)
    if len(raw)!=4 or struct.unpack("<I",raw)[0]!=target:
        raise VerificationError("original virtual slot pointer mismatch at "+hex(at))
def verify(reader):
    sha(reader,TICK,TICK_SIZE,TICK_SHA)
    sha(reader,SECONDARY_EVENT,SECONDARY_EVENT_SIZE,SECONDARY_EVENT_SHA)
    ptr32(reader,VTABLE+SLOT_OFFSET,TICK)
    ptr32(reader,VTABLE+SECONDARY_SLOT,SECONDARY_EVENT)
    for va,raw in WITNESSES:
        witness(reader,va,raw)
    for site in (0x22355645,0x22355654):
        rel32(reader,site,0xE8,WEAPON_UPDATE)
    for site in (0x2235564D,0x2235565C):
        rel32(reader,site,0xE9,PARENT_TICK)
    rel32(reader,SECONDARY_GATE_CALL,0xE8,SOURCE_GATE)
def verify_original(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
       hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("original owner 2006 retail DLL hashes mismatch")
    pe=PEImage(server)
    if pe.base!=0x22000000 or pe.section_of(TICK)!=".text" or (
        pe.section_of(SECONDARY_EVENT)!=".text"
    ) or pe.section_of(VTABLE+SLOT_OFFSET)!=".rdata":
        raise VerificationError("expected 2006 native server PE layout missing")
    verify(pe.at)
    return {
      "status":"ORIGINAL_2006_ELITE_WEAPON_TICK_PHASE631_VERIFIED",
      "server_sha256":SERVER_SHA256,
      "client_sha256":CLIENT_SHA256,
      "primary_elite_vtable":hex(VTABLE),
      "native_primary_vtable_tick_offset":hex(SLOT_OFFSET),
      "native_primary_vtable_tick_entry":hex(VTABLE+SLOT_OFFSET),
      "native_tick_va":hex(TICK),
      "native_tick_entire_function_bytes":TICK_SIZE,
      "native_tick_sha256":TICK_SHA,
      "first_and_second_serial_handle_field":hex(HANDLE),
      "original_timed_updater_va":hex(WEAPON_UPDATE),
      "exact_direct_update_call_sites":["0x22355645","0x22355654"],
      "original_inherited_grunt_tick_va":hex(PARENT_TICK),
      "exact_parent_tailjump_sites":["0x2235564d","0x2235565c"],
      "related_original_secondary_event_handler_va":hex(SECONDARY_EVENT),
      "related_secondary_event_entire_function_bytes":SECONDARY_EVENT_SIZE,
      "related_secondary_event_sha256":SECONDARY_EVENT_SHA,
      "related_elite_vtable_offset":hex(SECONDARY_SLOT),
      "secondary_gate_callsite":hex(SECONDARY_GATE_CALL),
      "actual_engine_callback_observed":False,
      "original_null_hazard":"After first serial resolution, a failed second lookup calls the updater with ECX=0.",
      "limits":"Function bodies authenticated but original game was not executed; secondary event handler is not yet ported."
    }
def synthetic_test():
    raw=bytes((n*37+11)%256 for n in range(119))
    expected=hashlib.sha256(raw).hexdigest()
    sha(lambda addr,n:raw[:n],0x71230000,len(raw),expected)
    for i in range(len(raw)):
        changed=bytearray(raw);changed[i]^=1
        try:sha(lambda addr,n:bytes(changed[:n]),0x71230000,len(raw),expected)
        except VerificationError:pass
        else:raise AssertionError("accepted altered original-free function")
    base=0x71240000
    table=0x71241000
    memory={
      table:struct.pack("<I",base),
      base:b"\x8b\x81\xdc\x0f\x00\x00",
    }
    ptr32(lambda addr,n:memory[addr][:n],table,base)
    witness(lambda addr,n:memory[addr][:n],base,"8b81dc0f0000")
    for original in (b"\xe8",b"\xe9"):
        target=0x71253000
        src=0x71252000
        opcode=original[0]
        edge=original+struct.pack("<i",target-src-5)
        rel32(lambda addr,n:edge[:n],src,opcode,target)
        for byte in range(5):
            changed=bytearray(edge);changed[byte]^=1
            try:rel32(lambda addr,n:bytes(changed[:n]),src,opcode,target)
            except VerificationError:pass
            else:raise AssertionError("accepted altered invented branch edge")
    try:verify_original(b"not original",b"not original")
    except VerificationError:pass
    else:raise AssertionError("accepted non-retail native DLL")
    print("PASS Phase631 original-free SHA, vtable, source witness, rel32 edge mutation and identity refusal")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--output",type=Path)
    a=p.parse_args()
    if a.self_test:
        synthetic_test()
        return 0
    if not a.server or not a.client:
        p.error("supply both owner-original server and client DLL paths or --self-test")
    try:
        report=verify_original(a.server.read_bytes(),a.client.read_bytes())
    except (VerificationError,ValueError,struct.error,OSError) as e:
        p.exit(1,"PHASE631 ORIGINAL ELITE TICK REFUSED: "+str(e)+"\n")
    blob=json.dumps(report,indent=2)+"\n"
    if a.output:a.output.write_text(blob,encoding="utf-8")
    else:print(blob,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
