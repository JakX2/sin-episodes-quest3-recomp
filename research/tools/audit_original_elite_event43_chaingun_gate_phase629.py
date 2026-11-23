#!/usr/bin/env python3
"""Phase629: verify full 2006 retail Elite event43 chaingun gate/tick bodies.
Owner-original SHA256 gate. --self-test uses invented bytes, no game files.
No proprietary machine-code functions are published by this tool.
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
GATE_VA=0x22315320
GATE_SIZE=82
GATE_SHA="0c0dda93a80bfdf697c92723bb3009406ec52c0e94befe7c1875ba8857d9c665"
TICK_VA=0x22315390
TICK_SIZE=173
TICK_SHA="13129eadd1c0023f308921188d4782da0a7f09debeb9dd98831776526bd56628"
TABLE_VA=0x22315374
TABLE_SIZE=16
TABLE_SHA="64a10f13fed6cb45ff28cb6757d5d6be0701fd0380f41dbdf2b4801ccbbafe69"
GATE_CASES=(0x22315335,0x2231536A,0x2231536E,0x2231536A)
ELITE_EVENT_CALL=0x22355503
CLOCK_GLOBAL=0x22572190
CLOCK_FLOAT_OFFSET=0x0C
THRESHOLD_VA=0x22477C5C
STATE_FIELD=0x594
START_TIME_FIELD=0x590
END_TIME_FIELD=0x58C
# Function-local original instruction pins. These are individual short
# witnesses (never a distributed original method body), supplemented by the
# immutable SHA256 of each *complete* original function and jump table.
OPCODE_WITNESSES=(
    (0x22315323,"8b8694050000"),
    (0x2231532E,"ff248574533122"),
    (0x22315337,"6a00"),
    (0x22315339,"6a0d"),
    (0x2231533D,"ff90c4030000"),
    (0x22315343,"c7869405000001000000"),
    (0x2231534D,"8b0d90215722"),
    (0x22315356,"899690050000"),
    (0x22315364,"898e8c050000"),
    (0x2231536A,"32c0"),
    (0x2231536E,"b001"),
    (0x22315393,"8b8694050000"),
    (0x22315399,"83e801"),
    (0x2231539E,"83e801"),
    (0x223153AE,"6a0d"),
    (0x223153B0,"ff90c8030000"),
    (0x223153BA,"6a0e"),
    (0x223153BE,"ff92c4030000"),
    (0x223153C4,"c7869405000000000000"),
    (0x223153D0,"a190215722"),
    (0x223153D8,"d8a68c050000"),
    (0x223153DE,"d81d5c7c4722"),
    (0x223153E6,"f6c401"),
    (0x223153EB,"c7869405000003000000"),
    (0x223153F7,"8b0d90215722"),
    (0x22315400,"d8a690050000"),
    (0x22315406,"d81d5c7c4722"),
    (0x2231540E,"f6c401"),
    (0x22315413,"c7869405000002000000"),
    (0x22315426,"898690050000"),
    (0x22315435,"89968c050000"),
    (0x2231543B,"5ec3"),
)
def verify_hash(reader,at,size,sha):
    raw=reader(at,size)
    if len(raw)!=size or hashlib.sha256(raw).hexdigest()!=sha:
        raise VerificationError("original complete gate/tick/table SHA mismatch")
def verify_witnesses(reader,witnesses=OPCODE_WITNESSES):
    for at,hx in witnesses:
        w=bytes.fromhex(hx)
        if reader(at,len(w))!=w:
            raise VerificationError("original gate/tick instruction mismatch at "+hex(at))
def verify_event_direct_call(reader,at=ELITE_EVENT_CALL,target=GATE_VA):
    raw=reader(at,5)
    if len(raw)!=5 or raw[0]!=0xE8 or (
       at+5+struct.unpack_from("<i",raw,1)[0]!=target
    ):
        raise VerificationError("Elite 0x43 native call target mismatch")
def verify_jump_table(reader):
    values=struct.unpack("<4I",reader(TABLE_VA,TABLE_SIZE))
    if values!=GATE_CASES:
        raise VerificationError("original gate switch cases changed")
def verify_original(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256
    ):
        raise VerificationError("owner retail 2006 SiN DLL pair required")
    p=PEImage(server)
    if p.base!=0x22000000 or p.section_of(GATE_VA)!=".text" or (
        p.section_of(TICK_VA)!=".text"
    ):
        raise VerificationError("original server.dll PE mapping differs")
    verify_hash(p.at,GATE_VA,GATE_SIZE,GATE_SHA)
    verify_hash(p.at,TICK_VA,TICK_SIZE,TICK_SHA)
    verify_hash(p.at,TABLE_VA,TABLE_SIZE,TABLE_SHA)
    verify_jump_table(p.at)
    verify_event_direct_call(p.at)
    verify_witnesses(p.at)
    if p.at(THRESHOLD_VA,4)!=struct.pack("<f",1.0):
        raise VerificationError("original gate/tick 1-second literal changed")
    if p.at(GATE_VA+GATE_SIZE,2)!=bytes((0x8B,0xFF)) or (
        p.at(TICK_VA+TICK_SIZE,1)!=bytes((0xCC,))
    ):
        raise VerificationError("native method boundary changed")
    return {
        "status":"OWNER_ORIGINAL_2006_ELITE_EVENT43_WEAPON_GATE_PHASE629_VERIFIED",
        "server_sha256":SERVER_SHA256,
        "client_sha256":CLIENT_SHA256,
        "complete_original_gate_va":hex(GATE_VA),
        "complete_original_gate_bytes":GATE_SIZE,
        "complete_original_gate_sha256":GATE_SHA,
        "complete_original_tick_va":hex(TICK_VA),
        "complete_original_tick_bytes":TICK_SIZE,
        "complete_original_tick_sha256":TICK_SHA,
        "additional_switch_table_data_bytes":TABLE_SIZE,
        "additional_switch_table_sha256":TABLE_SHA,
        "gate_switch_original_cases":[hex(x) for x in GATE_CASES],
        "original_event43_callsite":hex(ELITE_EVENT_CALL),
        "state_field":hex(STATE_FIELD),
        "clock_start_field":hex(START_TIME_FIELD),
        "clock_end_field":hex(END_TIME_FIELD),
        "global_engine_clock_ptr":hex(CLOCK_GLOBAL),
        "global_engine_clock_value_offset":hex(CLOCK_FLOAT_OFFSET),
        "one_second_x87_threshold":1.0,
        "original_phase_sequence":"0 gate to 1, timed update 1 to 2, timed update 2 to 3, update 3 to 0",
        "original_event43_permitted_only_in_phase":2,
        "short_instruction_witnesses":len(OPCODE_WITNESSES),
        "limitations":"Reference-only host callbacks; no actual Source entity, native game boot or Android gameplay proven."
    }
def self_test():
    source=bytes((x*19+7)%256 for x in range(90))
    digest=hashlib.sha256(source).hexdigest()
    verify_hash(lambda at,n:source[:n],0x71010000,len(source),digest)
    for i in range(len(source)):
        modified=bytearray(source);modified[i]^=1
        try:verify_hash(lambda at,n:bytes(modified[:n]),
                        0x71010000,len(source),digest)
        except VerificationError:pass
        else:raise AssertionError("accepted a changed synthetic function byte")
    memory={0x71020000:b"\x83\xc0\x02",0x71020010:b"\xff\x50\x04"}
    ws=((0x71020000,"83c002"),(0x71020010,"ff5004"))
    verify_witnesses(lambda at,n:memory[at][:n],ws)
    for at,hx in ws:
        for i in range(len(bytes.fromhex(hx))):
            changed=dict(memory);bad=bytearray(changed[at]);bad[i]^=1
            changed[at]=bytes(bad)
            try:verify_witnesses(lambda at,n:changed[at][:n],ws)
            except VerificationError:pass
            else:raise AssertionError("accepted changed synthetic opcode witness")
    fake_call=0x71030000;fake_target=0x71030100
    edge=b"\xe8"+struct.pack("<i",fake_target-fake_call-5)
    verify_event_direct_call(lambda at,n:edge[:n],fake_call,fake_target)
    for i in range(5):
        bad=bytearray(edge);bad[i]^=1
        try:verify_event_direct_call(lambda at,n:bytes(bad[:n]),
                                     fake_call,fake_target)
        except VerificationError:pass
        else:raise AssertionError("accepted changed original-free direct call")
    try:verify_original(b"wrong server",b"wrong client")
    except VerificationError:pass
    else:raise AssertionError("accepted nonretail binaries")
    print("PASS Phase629 complete-method SHA mutation, opcode/call edges and original-free refusal")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    if args.self_test:
        self_test();return 0
    if not args.server or not args.client:
        p.error("supply exact owner-original --server/--client DLLs or --self-test")
    try:report=verify_original(args.server.read_bytes(),
                               args.client.read_bytes())
    except (VerificationError,ValueError,struct.error,OSError) as e:
        p.exit(1,"ORIGINAL ELITE EVENT43 GATE REFUSED: "+str(e)+"\n")
    body=json.dumps(report,indent=2)+"\n"
    if args.output:args.output.write_text(body,encoding="utf-8")
    else:print(body,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
