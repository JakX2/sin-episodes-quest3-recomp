#!/usr/bin/env python3
"""Original-only full-SHA SiN 2006 gameplay triggers audit, with no original bytes output.

This verifies CTriggerDynamicDifficulty / CTriggerSecret factory registrations,
RTTI/vtable dispatch, complete touch/spawn method digests, DATADESC map keys,
and typed game-event helpers. It does NOT reconstruct engine-owned classes
or prove native Windows game startup. Production input must be the owner's
unmodified original 2006 server.dll. --self-test uses synthetic scalars only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
SERVER_BASE=0x22000000
REGISTRY_GETTER=0x2228B110
ORIGINAL_METHODS={
  "dynamic_touch":(0x221CCB70,137,"d8f847721d4465de7a3915bd2897933f82f33d16324c0e5975d4e2a7cbec5abe"),
  "secret_touch":(0x221CCC10,67,"9b7765825c74c2c7e59ba80143af3b833fa3ceb57c62b0844da18dfa49a2fc18"),
  "dynamic_spawn":(0x221D0740,69,"3309b7d25f963dbcb77ae2e1d6f8101fc97cf4ae0a1635c3e172d06d0f011c27"),
  "secret_spawn":(0x221D0790,99,"561260115c2318a26943c49e29d43494f7ef6ceea2108f12acafde9f27d8c329"),
  "dynamic_event_writer":(0x222B64D0,218,"ced0637d1c68f71408cb3836f6d6f4757fead802a02b6cef6cd36350d815db65"),
  "secret_game_event_writer":(0x222B63B0,103,"e2945d32f935b69d714477daece0bb9c7043213be53afc9e87c39ed8b58a4941"),
  "mark_difficulty_manager":(0x222B93E0,5,"de588dcd40cb4837e51bcf7c3a550256faafcae88cb9ab44e6df9c9ac3ac57bf"),
}
CLASSES={
  "trigger_dynamic_difficulty":(0x223BE4A0,0x2243CAF4,0x2250B404,
     0x2243B96C,0x224C3F84,0x2250D3FC,
     ".?AVCTriggerDynamicDifficulty@@",0x221CCB70,0x221D0740),
  "trigger_secret":(0x223BE4D0,0x2243CB10,0x2250B408,
     0x2243BC0C,0x224C3FE0,0x2250D424,
     ".?AVCTriggerSecret@@",0x221CCC10,0x221D0790),
}
FIELDS=(
    (0x2250CCFC,1,0x470,0x224391e8,"m_estimatedTimeToGetHere",
      0x224391d8,"EstimatedTime"),
    (0x2250CD30,5,0x474,0x224391c0,"m_estimatedDifficulty",
      0x224391ac,"EstimatedDifficulty"),
    (0x2250CD64,5,0x478,0x22439194,"m_finishesPlaythrough",
      0x22439188,"EndOfGame"),
)
class EvidenceRefused(ValueError):pass

class OriginalPE:
    def __init__(self,data:bytes):
        self.data=data
        if data[:2]!=b"MZ" or len(data)<0x100:
            raise EvidenceRefused("not a PE image")
        pe=struct.unpack_from("<I",data,0x3C)[0]
        if pe+24>len(data) or data[pe:pe+4]!=b"PE\0\0":
            raise EvidenceRefused("not a complete PE")
        machine,nsections=struct.unpack_from("<HH",data,pe+4)
        optlen=struct.unpack_from("<H",data,pe+20)[0]
        op=pe+24
        if machine!=0x14C or not 1<=nsections<=96 or (
                op+optlen+nsections*40>len(data)) or (
                struct.unpack_from("<H",data,op)[0]!=0x10B):
            raise EvidenceRefused("not original PE32 x86")
        self.base=struct.unpack_from("<I",data,op+28)[0]
        if self.base!=SERVER_BASE:raise EvidenceRefused("unexpected PE base")
        self.sections=[]
        for idx in range(nsections):
            off=op+optlen+idx*40
            name=data[off:off+8].split(b"\0")[0].decode("ascii","replace")
            size,va,rawsize,raw=struct.unpack_from("<IIII",data,off+8)
            self.sections.append((name,va,max(size,rawsize),raw,rawsize))
    def at(self,va:int,count:int)->bytes:
        if count<0 or count>4096:raise EvidenceRefused("invalid read size")
        rva=va-self.base
        for name,start,size,raw,rawsize in self.sections:
            if start<=rva and rva-start+count<=min(rawsize,size):
                off=raw+rva-start
                if off+count<=len(self.data):return self.data[off:off+count]
        raise EvidenceRefused(f"unmapped original RVA {rva:#x}")
    def u32(self,va:int)->int:
        return struct.unpack("<I",self.at(va,4))[0]
    def text(self,va:int,value:str)->bool:
        return self.at(va,len(value)+1)==value.encode("ascii")+b"\0"

def verify_datadesc(read_u32,read_text):
    for base,typ,offset,int_name,int_str,ext_name,ext_str in FIELDS:
        if (read_u32(base),read_u32(base+4),read_u32(base+8),
            read_u32(base+20))!=(typ,int_name,offset,ext_name):
            raise EvidenceRefused("original trigger DATADESC field shape mismatch")
        if not read_text(int_name,int_str) or not read_text(ext_name,ext_str):
            raise EvidenceRefused("original trigger DATADESC field key mismatch")
    return len(FIELDS)

def verify_raw(client:bytes,server:bytes)->dict:
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise EvidenceRefused("original client SHA-256 mismatch")
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
        raise EvidenceRefused("original server SHA-256 mismatch")
    pe=OriginalPE(server)
    for label,(va,length,expected) in ORIGINAL_METHODS.items():
        if hashlib.sha256(pe.at(va,length)).hexdigest()!=expected:
            raise EvidenceRefused("changed full original method "+label)
    for name,(register,name_ptr,factory,vtable,col,typedesc,rtti,touch,spawn) in CLASSES.items():
        if not pe.text(name_ptr,name):
            raise EvidenceRefused(name+" original class-name string mismatch")
        raw=pe.at(register,22)
        if not(raw[:1]==b"\xE8" and raw[5:8]==b"\x8B\x10\x68" and
               raw[12:13]==b"\x68" and raw[17:]==b"\x8B\xC8\xFF\x12\xC3"):
            raise EvidenceRefused(name+" factory registration not recognized")
        rel=struct.unpack_from("<i",raw,1)[0]
        if(register+5+rel,name_ptr,factory)!=(
            REGISTRY_GETTER,struct.unpack_from("<I",raw,8)[0],
            struct.unpack_from("<I",raw,13)[0]):
            raise EvidenceRefused(name+" factory pointer and registry mismatch")
        if pe.u32(vtable-4)!=col or pe.u32(col+12)!=typedesc:
            raise EvidenceRefused(name+" RTTI object locator mismatch")
        if not pe.text(typedesc+8,rtti):
            raise EvidenceRefused(name+" RTTI class string mismatch")
        if pe.u32(vtable+89*4)!=touch or pe.u32(vtable+25*4)!=spawn:
            raise EvidenceRefused(name+" virtual touch/spawn slot mismatch")
    count=verify_datadesc(pe.u32,pe.text)
    # Cross-check the exact event names/types/field labels the two
    # complete original event helpers use.
    names={
        0x2245C82C:"dynamic_difficulty",0x2245C824:"game",
        0x2245C84C:"descriptor",0x2245C858:"owner",
        0x2245C884:"advisor",0x2245C878:"decision",
        0x2240B13C:"name",0x223E0BBC:"value",
        0x22444D70:"x",0x22444D5C:"y",0x2245C88C:"z",
    }
    for va,label in names.items():
        if not pe.text(va,label):
            raise EvidenceRefused("original event field witness changed "+label)
    return {
        "status":"ORIGINAL_2006_TRIGGER_GAMEPLAY_EVIDENCE_VERIFIED",
        "full_native_method_hashes":len(ORIGINAL_METHODS),
        "full_original_method_bytes":sum(x[1] for x in ORIGINAL_METHODS.values()),
        "registered_trigger_classnames":list(CLASSES),
        "original_datadesc_keyvalues":[row[6] for row in FIELDS],
        "dynamic_touch_virtual_slot":89,
        "secret_touch_virtual_slot":89,
        "original_event_names":["dynamic_difficulty","game"],
        "original_source_hashes":{"client":CLIENT_SHA,"server":SERVER_SHA},
        "limitations":"No real game entity creation, map load, AI or playable DLL Init."
    }

def self_test()->None:
    values={}
    texts={}
    for base,typ,offset,name,name_str,external,external_str in FIELDS:
        values.update({base:typ,base+4:name,base+8:offset,base+20:external})
        texts.update({name:name_str,external:external_str})
    assert verify_datadesc(lambda va:values.get(va,0),
                           lambda va,s:texts.get(va)==s)==3
    for base,*_ in FIELDS:
        bad=dict(values);bad[base+8]+=4
        try:verify_datadesc(lambda va:bad.get(va,0),
                            lambda va,s:texts.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated original field offset accepted")
    for key in list(texts):
        bad=dict(texts);bad[key]="invented"
        try:verify_datadesc(lambda va:values.get(va,0),
                            lambda va,s:bad.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("forged original map key accepted")
    try:verify_raw(b"synthetic",b"synthetic")
    except EvidenceRefused:pass
    else:raise AssertionError("synthetic DLL accepted as original")
    print("PASS original-free 3 DATADESC field/offset/name mutations and original SHA refusal")

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client",type=Path)
    ap.add_argument("--server",type=Path)
    ap.add_argument("--self-test",action="store_true")
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    if args.self_test:
        self_test();return 0
    if not args.client or not args.server:
        ap.error("original --client and --server are both required")
    try:
        result=verify_raw(args.client.read_bytes(),args.server.read_bytes())
    except (OSError,EvidenceRefused,struct.error) as e:
        ap.exit(1,"ORIGINAL TRIGGER EVIDENCE REFUSED: "+str(e)+"\n")
    out=json.dumps(result,indent=2)+"\n"
    if args.output:args.output.write_text(out)
    else:print(out,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
