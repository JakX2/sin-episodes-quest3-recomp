#!/usr/bin/env python3
"""Phase622: SHA-gated original 2006 CNPC_EliteGrunt factory reconstruction.

Pure audit of the exact owner-supplied unmodified client.dll/server.dll pair.
Does not dump x86 code, class objects or proprietary runtime resources.
Synthetic --self-test has independently fabricated memory and no game bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256,
    REGISTRY_GETTER
)
from audit_sin_original_npc_factories import (
    NPC_FACTORIES,verify_registration_shape
)

CLASSNAME="npc_druglab_grunt_elite"
REGISTER_VA=0x223C6940
FACTORY_OBJECT=0x225346A0
FACTORY_VTABLE=0x2248DB64
FACTORY_COL=0x224D1710
FACTORY_TYPE_DESCRIPTOR=0x225346A4
FACTORY_RTTI=".?AV?$CEntityFactory@VCNPC_EliteGrunt@@@@"
NATIVE_OBJECT_BYTES=0xFE0
NETWORKABLE_SUBOBJECT_OFFSET=0x160
UNKNOWN_EXTRA_SENTINEL_OFFSET=0xFDC
# All complete source methods end before canonical 0xCC padding.
METHODS=(
    ("elite_factory_creator_wrapper",0x22355C70,23,
     "6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9"),
    ("elite_factory_alloc_and_dispatch",0x22355C90,61,
     "1a5252daa1697cb3309a38c3581f0ee6400b4a412a8f6518c598635665489ae0"),
    ("elite_npc_constructor",0x22355CD0,90,
     "309d176029f698699be3888311b0d93713a952e24d503622227290dc9923ca32"),
    ("elite_factory_delegated_destroy",0x223552C0,16,
     "ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21"),
)
ORIGINAL_DIRECT_CALLS=(
    (0x22355C77,0x22355C90),
    (0x22355C96,0x22138940),
    (0x22355CA4,0x22355CD0),
    (0x22355CD3,0x22360D80),
    (0x22355CF2,0x22360BC0),
    (0x22355D04,0x22355A20),
)
# 2006 original PE32 vptrs: inspect for provenance ONLY; the new
# portable C++ model never writes these virtual table addresses.
NATIVE_VPTRS=(
    (0x22355CD8,0x0,0x2248DF04),
    (0x22355CDE,0x790,0x2248DED8),
    (0x22355CE8,0xC14,0x2248DE9C),
)

def u32(pe,at):
    return struct.unpack("<I",pe.at(at,4))[0]

def verify_method_hashes(reader,methods=METHODS):
    for name,va,length,expected in methods:
        body=reader(va,length)
        if len(body)!=length or hashlib.sha256(body).hexdigest()!=expected:
            raise VerificationError(f"complete original Elite method differs: {name}")
    return len(methods)

def verify_calls(reader,calls=ORIGINAL_DIRECT_CALLS):
    for at,target in calls:
        data=reader(at,5)
        if len(data)!=5 or data[:1]!=b"\xe8":
            raise VerificationError("Elite factory CALL opcode changed")
        rel=struct.unpack_from("<i",data,1)[0]
        if at+5+rel!=target:
            raise VerificationError("Elite factory CALL target changed")
    return len(calls)

def verify_vptrs(reader,words,sites=NATIVE_VPTRS):
    for at,offset,expected in sites:
        if offset==0:
            if reader(at,2)!=b"\xc7\x06" or words(at+2)!=expected:
                raise VerificationError("Elite primary vptr assignment changed")
        else:
            if reader(at,2)!=b"\xc7\x86" or words(at+2)!=offset or (
               words(at+6)!=expected):
                raise VerificationError("Elite nested vptr assignment changed")
    return len(sites)

def verify_original(client,server):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256
    ):
        raise VerificationError("incorrect original 2006 retail DLL pair")
    pe=PEImage(server)
    if pe.base!=0x22000000:
        raise VerificationError("unsupported original Elite PE32 image base")
    w=lambda at:u32(pe,at)
    if pe.section_of(FACTORY_OBJECT)!=".data" or (
        pe.section_of(FACTORY_VTABLE)!=".rdata"
    ):
        raise VerificationError("Elite factory object/vtable section changed")
    if (w(FACTORY_OBJECT),w(FACTORY_VTABLE-4),w(FACTORY_VTABLE),
        w(FACTORY_VTABLE+4))!=(
        FACTORY_VTABLE,FACTORY_COL,0x22355C70,0x223552C0
    ):
        raise VerificationError("Elite factory vtable/delegate changed")
    if w(FACTORY_COL+12)!=FACTORY_TYPE_DESCRIPTOR or (
        pe.at(FACTORY_TYPE_DESCRIPTOR+8,len(FACTORY_RTTI)+1)
        !=FACTORY_RTTI.encode("ascii")+b"\0"
    ):
        raise VerificationError("original Elite factory class RTTI differs")
    elite_reg=[x for x in NPC_FACTORIES if x[0]==CLASSNAME]
    if len(elite_reg)!=1:
        raise VerificationError("original Phase530 Elite registration missing")
    classname,registered,string_va,factory_va,registrar_sha=elite_reg[0]
    if registered!=REGISTER_VA or factory_va!=FACTORY_OBJECT:
        raise VerificationError("original Elite registration metadata changed")
    if pe.at(string_va,len(classname)+1)!=classname.encode()+b"\0":
        raise VerificationError("original Elite registered classname changed")
    registrar=pe.at(registered,22)
    if hashlib.sha256(registrar).hexdigest()!=registrar_sha:
        raise VerificationError("original Phase530 Elite registrar differs")
    verify_registration_shape(registrar,registered,string_va,factory_va,REGISTRY_GETTER)

    count=verify_method_hashes(pe.at)
    verify_calls(pe.at)
    verify_vptrs(pe.at,w)
    # Separate from MobGrunt: Elite allocates 0xFE0 bytes, not 0xFD8.
    if pe.at(0x22355C91,5)!=b"\x68\xe0\x0f\x00\x00":
        raise VerificationError("Elite native allocation size differs")
    if pe.at(0x22355C7F,5)!=b"\x05\x60\x01\x00\x00":
        raise VerificationError("Elite factory +0x160 returned view differs")
    if pe.at(0x22355CB4,3)!=b"\xff\x50\x70":
        raise VerificationError("Elite native initializer virtual70 missing")
    # Additional per-instance value appears after common postinit. Its
    # gameplay semantic name is NOT verified; only field offset and bits.
    if pe.at(0x22355D1C,6)!=b"\xc7\x86\xdc\x0f\x00\x00" or (
        w(0x22355D22)!=0xffffffff
    ):
        raise VerificationError("Elite original unknown +0xFDC sentinel changed")
    # The original constructor calls a common static initialization first,
    # then checks a generation word, conditionally invokes Elite static init,
    # publishes both the flag and cached epoch, and writes +0xFDC.
    if (pe.at(0x22355CF7,1)!=b"\xa1" or w(0x22355CF8)!=0x22534694
        or pe.at(0x22355CFC,2)!=b"\x3b\x05"
        or w(0x22355CFE)!=0x2254A3C0
        or pe.at(0x22355D02,2)!=b"\x74\x18"
        or pe.at(0x22355D0F,2)!=b"\xc6\x05"
        or w(0x22355D11)!=0x22534690
        or pe.at(0x22355D15,1)!=b"\x01"
        or pe.at(0x22355D16,2)!=b"\x89\x0d"
        or w(0x22355D18)!=0x22534694):
        raise VerificationError("Elite static epoch/flag guard changed")
    if pe.at(0x223552C0,9)!=b"\x8b\x4c\x24\x04\x85\xc9\x74\x05\x8b":
        raise VerificationError("Elite delegate null-guard changed")
    return {
        "status":"ORIGINAL_2006_ELITE_GRUNT_FACTORY_VERIFIED",
        "original_server_sha256":SERVER_SHA256,
        "original_client_sha256":CLIENT_SHA256,
        "original_registered_classname":CLASSNAME,
        "factory_rtti":FACTORY_RTTI,
        "factory_object_va":hex(FACTORY_OBJECT),
        "factory_vtable_va":hex(FACTORY_VTABLE),
        "authenticated_complete_native_methods":[
            {"name":name,"original_va":hex(va),"bytes":size,"sha256":digest}
            for name,va,size,digest in METHODS
        ],
        "native_object_bytes":NATIVE_OBJECT_BYTES,
        "returned_networkable_offset":hex(NETWORKABLE_SUBOBJECT_OFFSET),
        "extra_field_offset":hex(UNKNOWN_EXTRA_SENTINEL_OFFSET),
        "extra_field_written_value":"0xffffffff",
        "original_vptr_fields":[
            {"offset":hex(offset),"original_vtable_va":hex(vtable)}
            for _,offset,vtable in NATIVE_VPTRS
        ],
        "original_direct_calls":len(ORIGINAL_DIRECT_CALLS),
        "epoch_cache_va":"0x22534694",
        "epoch_source_va":"0x2254a3c0",
        "conditional_epoch_refresh_va":"0x22355a20",
        "note":"No original DLL bytes exported. Real NPC/AI/engine host remains absent."
    }

def self_test():
    fakeva=0x71001000
    fake_methods=[("a",fakeva,4,hashlib.sha256(b"abcd").hexdigest())]
    assert verify_method_hashes(lambda a,n:b"abcd"[:n],fake_methods)==1
    for altered in (b"Abcd",b"abcD",b"abc"):
        try:
            verify_method_hashes(lambda a,n:altered[:n],fake_methods)
        except VerificationError:pass
        else:raise AssertionError("forged synthetic full method accepted")
    calls=((0x71001000,0x71004000),(0x71001020,0x71005000))
    words={at:b"\xe8"+struct.pack("<i",target-at-5)
           for at,target in calls}
    get=lambda at,n:words[at][:n]
    assert verify_calls(get,calls)==2
    for at in words:
        for byte in range(5):
            modified=dict(words)
            corrupt=bytearray(words[at]);corrupt[byte]^=1
            modified[at]=bytes(corrupt)
            try:verify_calls(lambda p,n:modified[p][:n],calls)
            except VerificationError:pass
            else:raise AssertionError("altered synthetic CALL accepted")
    # Fabricated (distinct!) layout and vptrs exercise role validation.
    vptrs=((0x71003000,0,0x72001000),
           (0x71003010,0x790,0x72002000),
           (0x71003020,0xC14,0x72003000))
    parts={
        site:(b"\xc7\x06"+struct.pack("<I",value)) if offset==0
        else (b"\xc7\x86"+struct.pack("<II",offset,value))
        for site,offset,value in vptrs
    }
    def read(at,n):
        for site,data in parts.items():
            if site<=at<site+len(data):return data[at-site:at-site+n]
        return b""
    assert verify_vptrs(read,lambda at:struct.unpack("<I",read(at,4))[0],vptrs)==3
    for site in parts:
        for i in range(len(parts[site])):
            altered=dict(parts)
            raw=bytearray(parts[site]);raw[i]^=1;altered[site]=bytes(raw)
            def corrupted(at,n):
                for base,data in altered.items():
                    if base<=at<base+len(data):return data[at-base:at-base+n]
                return b""
            try:
                verify_vptrs(corrupted,lambda at:struct.unpack(
                    "<I",corrupted(at,4))[0],vptrs)
            except VerificationError:pass
            else:raise AssertionError("mutated synthetic vptr write accepted")
    try:verify_original(b"fake-client",b"fake-server")
    except VerificationError:pass
    else:raise AssertionError("untrusted original Elite binaries accepted")
    print("PASS Phase622 fully synthetic Elite method digests, direct CALL edges, native vptr assignments and original SHA rejection")

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client",type=Path)
    ap.add_argument("--server",type=Path)
    ap.add_argument("--output",type=Path)
    ap.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:
        self_test();return 0
    if args.client is None or args.server is None:
        ap.error("exact owner-held original --client and --server DLLs required")
    try:
        r=verify_original(args.client.read_bytes(),args.server.read_bytes())
    except (VerificationError,struct.error,OSError) as ex:
        ap.exit(1,"ELITE GRUNT SOURCE VERIFICATION REFUSED: "+str(ex)+"\n")
    out=json.dumps(r,indent=2)+"\n"
    if args.output:args.output.write_text(out,encoding="utf-8")
    else:print(out,end="")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
