#!/usr/bin/env python3
"""Verify two original 2006 SiN grenade AI sound events against private server.dll.

Read only. Exact 2006 PE32 SHA is required. Recovered machine-code metadata is
NOT a reconstructed Source ABI, audible sound, or Quest gameplay module.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"

def u32(n: int) -> bytes:
    return struct.pack("<I",n)

# Original compiled x86 instructions, not a Source SDK reimplementation.
INSTRUCTIONS={
    0x2231436B:b"\xd8\x05"+u32(0x223D9CB0),  # next think +.05
    0x2231444A:bytes.fromhex("d986c4070000"), # field +7C4
    0x22314450:b"\xd8\x1d"+u32(0x224771FC), # compare 300
    0x22314463:b"\xdc\x05"+u32(0x22467448), # increment 15
    0x2231448A:b"\xd8\x0d"+u32(0x223D9778), # velocity*0.5
    0x223144E5:b"\x6a\x01",                    # channel 1
    0x223144ED:b"\x56",                        # grenade pointer
    0x223144F2:b"\x68"+u32(0x3E4CCCCD),       # duration .2
    0x22314537:bytes.fromhex("d986c4070000"), # radius from +7C4
    0x22314557:b"\x6a\x08",                    # SOUND_DANGER
    0x22314CED:b"\xc7\x86\xc4\x07\x00\x00"+struct.pack("<f",100.0),
    0x221B8FA0:b"\x8b\x0d"+u32(0x2256E844), # AI sound manager global
    0x222A5E39:bytes.fromhex("ff90b8000000"), # game rules damage virtual
    0x222A5E3F:bytes.fromhex("f6463840"), # damage type 0x40 DMG_BLAST
    0x222A5E49:bytes.fromhex("d9442418"), # radius
    0x222A5E4D:b"\xdc\x0d"+u32(0x223DE5D0), # multiply 1.5
    0x222A5E53:b"\xdd\x05"+u32(0x224562D8), # minimum 128
    0x222A5E95:b"\x6a\x00", # channel 0
    0x222A5EA4:b"\x68"+u32(0x3F800000), # duration 1.0
    0x222A5EBF:b"\x68"+u32(0x02000001), # COMBAT|CONTEXT_EXPLOSION
}
CALLS={0x22314559:0x221B8FA0,0x222A5EC4:0x221B8FA0}
FLOATS={0x223D9CB0:0.05,0x223D9778:0.5,0x224771FC:300.0}
DOUBLES={0x22467448:15.0,0x223DE5D0:1.5,0x224562D8:128.0}

class EvidenceError(ValueError):
    pass

class PE32:
    def __init__(self,data:bytes):
        self.data=data
        if len(data)<0x100 or data[:2]!=b"MZ":
            raise EvidenceError("invalid DOS image")
        nt=struct.unpack_from("<I",data,0x3c)[0]
        if nt+24>len(data) or data[nt:nt+4]!=b"PE\x00\x00":
            raise EvidenceError("invalid PE signature")
        machine,sec_count=struct.unpack_from("<HH",data,nt+4)
        optsize=struct.unpack_from("<H",data,nt+20)[0]
        opt=nt+24
        if (machine!=0x14c or not 1<=sec_count<=32 or
            opt+optsize+40*sec_count>len(data) or
            struct.unpack_from("<H",data,opt)[0]!=0x10b):
            raise EvidenceError("expected bounded x86 PE32")
        self.base=struct.unpack_from("<I",data,opt+28)[0]
        self.sections=[]
        for i in range(sec_count):
            entry=opt+optsize+40*i
            name=data[entry:entry+8].split(b"\0")[0].decode("ascii","replace")
            _,rva,rawsize,rawoff=struct.unpack_from("<IIII",data,entry+8)
            if rawoff>len(data) or rawsize>len(data)-rawoff:
                raise EvidenceError("out of range PE section")
            self.sections.append((name,rva,rawsize,rawoff))

    def at(self,va:int,count:int,section:str)->bytes:
        for name,rva,size,offset in self.sections:
            delta=va-self.base-rva
            if name==section and count>=0 and 0<=delta<=size-count:
                return self.data[offset+delta:offset+delta+count]
        raise EvidenceError(f"unmapped original VA {va:#x} in {section}")

def audit(server:bytes,client:bytes|None=None,*,check_hash:bool=True)->dict:
    digest=hashlib.sha256(server).hexdigest()
    if check_hash and digest!=SERVER_SHA:
        raise EvidenceError("original server.dll SHA-256 mismatch")
    pe=PE32(server)
    if pe.base!=0x22000000:
        raise EvidenceError("not original SiN x86 server image base")
    for address,raw in INSTRUCTIONS.items():
        if pe.at(address,len(raw),".text")!=raw:
            raise EvidenceError(f"original instruction mismatch at {address:#x}")
    for site,target in CALLS.items():
        code=pe.at(site,5,".text")
        if code[0]!=0xe8 or site+5+struct.unpack_from("<i",code,1)[0]!=target:
            raise EvidenceError(f"original sound insert CALL mismatch at {site:#x}")
    for address,value in FLOATS.items():
        if pe.at(address,4,".rdata")!=struct.pack("<f",value):
            raise EvidenceError(f"original float mismatch at {address:#x}")
    for address,value in DOUBLES.items():
        if pe.at(address,8,".rdata")!=struct.pack("<d",value):
            raise EvidenceError(f"original double mismatch at {address:#x}")
    client_info=None
    if client is not None:
        ch=hashlib.sha256(client).hexdigest()
        if check_hash and ch!=CLIENT_SHA:
            raise EvidenceError("original client.dll SHA-256 mismatch")
        client_info={"sha256":ch,
                     "DT_BaseGrenade_name_present":b"DT_BaseGrenade\0" in client,
                     "DT_GrenadeAR_name_present":b"DT_GrenadeAR\0" in client,
                     "warning":"Strings alone do not establish a full network wire contract."}
    return {
        "original_server_sha256":digest,
        "evidence":{"x86_instruction_sites":len(INSTRUCTIONS),
                    "verified_sound_insert_CALLs":len(CALLS),
                    "float_sites":len(FLOATS),"double_sites":len(DOUBLES)},
        "grenade_flight_AI_warning":{
            "source_class":"CGrenadeAR","think_va":"0x22314360",
            "sound_insert_va":"0x221B8FA0","callsite_va":"0x22314559",
            "type_bits":"0x00000008","interpretation":"SOUND_DANGER",
            "sound_origin":"grenade origin + grenade velocity * 0.5",
            "radius_field_x86":"+0x7C4","initial_radius":100.0,
            "conditional_step":15.0,"compare_radius_to":300.0,
            "tick_seconds":struct.unpack("<f",struct.pack("<f",.05))[0],
            "event_duration":struct.unpack("<f",struct.pack("<f",.2))[0],
            "channel":1},
        "blast_AI_sound_after_radius_damage":{
            "damage_dispatch_va":"0x222A5E10",
            "game_rules_virtual_damage_callsite":"0x222A5E39",
            "blast_flag_test_va":"0x222A5E3F",
            "requires_damage_type_bit":"0x40",
            "sound_insert_va":"0x221B8FA0","callsite_va":"0x222A5EC4",
            "type_bits":"0x02000001",
            "interpretation":"SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION",
            "radius_rule":"max(128.0, explosion_damage_radius * 1.5)",
            "minimum_radius":128.0,"radius_multiplier":1.5,
            "event_duration":1.0,"channel":0},
        "client_string_crosscheck":client_info,
        "limits":[
            "Original x86 bitmask and instruction evidence, with Source SDK names corroborating it.",
            "AI sound system events are NOT audible or visually networked client effects.",
            "The radius growth comparison is not a proven hard cap and may overshoot 300.",
            "Do not substitute historical SDK .25-second event duration for original SiN 1.0.",
            "The original 2006 server code cannot be called directly by ARM64 Quest game modules.",
        ],
    }

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    try:
        result=audit(a.server.read_bytes(),
                     a.client.read_bytes() if a.client else None)
        a.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError,struct.error) as e:
        p.exit(1,"SiN grenade AI sound audit failed: "+str(e)+"\n")
    print(json.dumps(result["evidence"],indent=2))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
