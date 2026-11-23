#!/usr/bin/env python3
"""Source-authenticate the complete original SiN Elite +0x44C event handler.

Requires BOTH unchanged 2006 retail DLL SHA256s. Pins the complete 232-byte
method, vtable, 12 original direct x86 call edges, and critical opcode fields.
CI --self-test only mutates invented stand-in bytes. No original method body
or proprietary game DLL is committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256
)
VA=0x223556A0
SIZE=232
SHA="66a3abbe73c87aaa6ea5f7c9c97547b99a4c2fe6e626c6bb1f77c1a8a8c60262"
VTABLE=0x2248DF04
SLOT=0x44C
INHERITED=0x2235CCE0
GATE=0x22315320
CALLS=(
    (0x223556BA,INHERITED,"inherited_event"),
    (0x223556C5,0x220326D0,"event47_predicate"),
    (0x223556D4,0x222AA640,"event47_first_lookup"),
    (0x223556E3,0x222AA640,"event47_second_lookup"),
    (0x223556FD,0x222AA640,"event47_third_lookup"),
    (0x2235570A,0x2203F8E0,"event47_component"),
    (0x22355713,0x220398A0,"event47_command3"),
    (0x2235571C,0x220398A0,"event47_command4"),
    (0x22355725,0x220398A0,"event47_command5"),
    (0x2235572E,0x2203D2D0,"event47_action_zero"),
    (0x22355772,GATE,"event43_weapon_gate"),
    (0x2235577F,0x2203D2D0,"event43_action_zero"),
)
WITNESSES=(
    (0x223556A0,"568bf1"), # native event handler prolog
    (0x223556A3,"8b4c2408"), # original event pointer at [ESP+8] after push
    (0x223556A7,"8b01"), # event ID is first event DWORD
    (0x223556A9,"83f843"), # event 0x43
    (0x223556B2,"83f847"), # event 0x47
    (0x223556B7,"51"), # original event ptr pushed to inherited
    (0x223556E8,"8b10"),
    (0x223556EC,"ff9204040000"), # dynamic object virtual +0x404
    (0x223556F2,"89442408"), # copy returned EAX into local stack
    (0x223556F6,"8d442408"), # pass pointer to that local value
    (0x22355704,"81c1ec040000"), # target object +0x4EC
    (0x2235570F,"6a03"),
    (0x22355718,"6a04"),
    (0x22355721,"6a05"),
    (0x2235572C,"6a00"),
    (0x22355737,"80bed90f000000"), # Elite predicate FD9
    (0x22355744,"8b86dc0f0000"), # Elite handle FDC
    (0x2235574A,"83f8ff"),
    (0x2235574F,"8b1520564f22"), # global serial table
    (0x22355757,"81e1ff0f0000"), # 12-bit slot
    (0x2235575D,"c1e104"), # 16-byte serial slot stride
    (0x22355764,"c1e80c"), # high generation bits
    (0x22355767,"394104"), # validate serial
    (0x2235576C,"8b09"), # load resolved target
    (0x22355770,"33c9"), # null fallback before native gate
    (0x22355777,"84c0"), # gate return controls action
    (0x2235577D,"6a00"),
    (0x22355784,"5ec20400"), # cleanup and ret 4
)
def verify_sha(reader,va,size,digest):
    raw=reader(va,size)
    if len(raw)!=size or hashlib.sha256(raw).hexdigest()!=digest:
        raise VerificationError("complete original event method SHA mismatch")
def verify_witness(reader,va,hexbytes):
    data=bytes.fromhex(hexbytes)
    if reader(va,len(data))!=data:
        raise VerificationError("original opcode mismatch at "+hex(va))
def verify_pointer(reader,slot,value):
    raw=reader(slot,4)
    if len(raw)!=4 or struct.unpack("<I",raw)[0]!=value:
        raise VerificationError("original Elite vtable slot changed")
def verify_call(reader,va,target):
    raw=reader(va,5)
    if len(raw)!=5 or raw[0]!=0xE8 or (
        va+5+struct.unpack_from("<i",raw,1)[0]!=target):
        raise VerificationError("original direct event call changed at "+hex(va))
def verify(reader):
    verify_sha(reader,VA,SIZE,SHA)
    verify_pointer(reader,VTABLE+SLOT,VA)
    for site,target,_ in CALLS:
        verify_call(reader,site,target)
    for site,operand in WITNESSES:
        verify_witness(reader,site,operand)
def verify_original(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
       hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("both owner original retail 2006 DLL hashes required")
    p=PEImage(server)
    if p.base!=0x22000000 or p.section_of(VA)!=".text" or (
        p.section_of(VTABLE+SLOT)!=".rdata"):
        raise VerificationError("retail original PE32 sections mismatch")
    verify(p.at)
    return {
       "status":"ORIGINAL_2006_ELITE_SECONDARY_EVENT_PHASE632_VERIFIED",
       "original_server_sha256":SERVER_SHA256,
       "original_client_sha256":CLIENT_SHA256,
       "native_vtable_slot_va":hex(VTABLE+SLOT),
       "native_vtable_offset":hex(SLOT),
       "original_event_handler_va":hex(VA),
       "entire_original_method_bytes":SIZE,
       "entire_original_method_sha256":SHA,
       "event_argument":"original [esp+4] pointer at entry; first DWORD event ID",
       "source_supported_event_ids":[hex(0x43),hex(0x47)],
       "inherited_event_handler_va":hex(INHERITED),
       "gate_va":hex(GATE),
       "verified_direct_call_edges":[
          {"site":hex(site),"target":hex(target),"role":label}
          for site,target,label in CALLS
       ],
       "separate_object_lookups_event47":3,
       "numbered_original_calls_event47":[3,4,5],
       "native_null_hazard":"0x43 calls gate on null serial target; 0x47 second/third lookups dereferenced without null checks",
       "native_gameplay_observed":False,
       "limits":"CI tests are pure synthetic hosts; owner game process has not run this reconstructed code."
    }
def synthetic_test():
    fake=bytes((n*23+5)%256 for n in range(240))
    digest=hashlib.sha256(fake).hexdigest()
    verify_sha(lambda va,n:fake[:n],0x71001000,len(fake),digest)
    for index in range(len(fake)):
        changed=bytearray(fake);changed[index]^=1
        try:verify_sha(lambda va,n:bytes(changed[:n]),
                       0x71001000,len(fake),digest)
        except VerificationError:pass
        else:raise AssertionError("mutated original-free method SHA accepted")
    slot=0x71002000;target=0x71003000
    ptr=struct.pack("<I",target)
    verify_pointer(lambda va,n:ptr[:n],slot,target)
    for i in range(4):
        changed=bytearray(ptr);changed[i]^=1
        try:verify_pointer(lambda va,n:bytes(changed[:n]),slot,target)
        except VerificationError:pass
        else:raise AssertionError("mutated invented virtual slot accepted")
    site=0x71004000;destination=0x71005000
    call=b"\xe8"+struct.pack("<i",destination-site-5)
    verify_call(lambda va,n:call[:n],site,destination)
    for i in range(len(call)):
        changed=bytearray(call);changed[i]^=1
        try:verify_call(lambda va,n:bytes(changed[:n]),site,destination)
        except VerificationError:pass
        else:raise AssertionError("mutated invented CALL accepted")
    verify_witness(lambda va,n:b"\x83\xf8\x47"[:n],site,"83f847")
    try:verify_witness(lambda va,n:b"\x83\xf8\x43"[:n],site,"83f847")
    except VerificationError:pass
    else:raise AssertionError("changed event selector accepted")
    try:verify_original(b"not retail",b"not retail")
    except VerificationError:pass
    else:raise AssertionError("unverified originals accepted")
    print("PASS Phase632: complete native method SHA, original vtable, direct call, opcode mutation refusals")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    a=p.parse_args()
    if a.self_test:
        synthetic_test();return 0
    if not a.server or not a.client:
        p.error("provide exact original --server and --client, or --self-test")
    try:
        report=verify_original(a.server.read_bytes(),a.client.read_bytes())
    except (VerificationError,OSError,ValueError,struct.error) as e:
        p.exit(1,"ORIGINAL PHASE632 EVENT AUDIT REFUSED: "+str(e)+"\n")
    blob=json.dumps(report,indent=2)+"\n"
    if a.output:a.output.write_text(blob,encoding="utf-8")
    else:print(blob,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
