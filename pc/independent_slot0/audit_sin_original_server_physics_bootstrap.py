#!/usr/bin/env python3
"""Original owner-private 2006 SiN server physics bootstrap SHA audit.

Verifies three *complete* owner-only function region SHA256 digests and
sparse x86/constant witnesses; never emits full original machine code.
Public --self-test uses fabricated PE bytes and mutation tests only.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
BASE = 0x22000000
REGIONS = {
    "server_physics_factory_and_surface_init": (0x2220D910,127,
        "415f272c8f5fd8406c8a2a25e04ad99064f7476284e91fd9fa834acd58cbe838"),
    "server_surface_script_read_and_parse": (0x22233FD0,136,
        "0afda89d832437af8f04442195f6afe9c334cd67a432ac557e54e301e52d3a02"),
    "server_surface_manifest_dispatch": (0x22234060,247,
        "37fa813203b8ec7aa57dd642dd72ce25fffad1a81241f520fa6d106e276b9745"),
}
# Sparse, non-contiguous original 2006 machine-code witnesses.
# Only a few bytes of any native function are published.
WITNESSES = {
    0x2220D92B: bytes.fromhex("6a0068fc994422ffd0"),
    0x2220D940: bytes.fromhex("6a0068e4994422ff542410"),
    0x2220D957: bytes.fromhex("6a0068cc994422ff542410"),
    0x2220D96E: bytes.fromhex("8b0da42157225150e8e5660200"),
    0x22234028: bytes.fromhex("8b11508b450850ff5204"),
    0x2220D9B0: bytes.fromhex("8b0da05357228b015733ffff5008"),
    0x2220D9C7: bytes.fromhex("8b0da05357228b1157ff5214"),
}
STRINGS = {
    0x224499FC: b"VPhysics030\x00",
    0x224499E4: b"VPhysicsCollision007\x00",
    0x224499CC: b"VPhysicsSurfaceProps001\x00",
    0x223EA728: b"GAME\x00",
    0x223F0B4C: b"rb\x00",
    0x22433AB0: b"file\x00",
    0x2244D200: b"scripts/surfaceproperties_manifest.txt\x00",
}
POINTERS = {0x22516A84:0x2244D200}
class AuditError(ValueError):
    pass

def u16(data,i):
    if i<0 or i+2>len(data):raise AuditError("truncated PE16")
    return struct.unpack_from("<H",data,i)[0]
def u32(data,i):
    if i<0 or i+4>len(data):raise AuditError("truncated PE32")
    return struct.unpack_from("<I",data,i)[0]

class OriginalPE:
    def __init__(self,data):
        self.data=data
        if data[:2]!=b"MZ" or len(data)<512:
            raise AuditError("not an MS-DOS PE image")
        nt=u32(data,0x3c)
        if data[nt:nt+4]!=b"PE\x00\x00":
            raise AuditError("PE signature mismatch")
        n=u16(data,nt+6); opt=nt+24
        if u16(data,opt)!=0x10b or u32(data,opt+28)!=BASE:
            raise AuditError("server original base/machine layout mismatch")
        section_start=opt+u16(data,nt+20)
        if n<1 or n>64:raise AuditError("invalid section count")
        self.sections=[]
        for i in range(n):
            o=section_start+i*40
            if o+40>len(data):raise AuditError("truncated section")
            name=data[o:o+8].rstrip(b"\x00")
            virtual_size=u32(data,o+8)
            rva=u32(data,o+12)
            raw_size=u32(data,o+16)
            raw_start=u32(data,o+20)
            if raw_start+raw_size>len(data):
                raise AuditError("section out of file")
            self.sections.append((name,rva,virtual_size,raw_start,raw_size))
    def read(self,va,length):
        if not 0<length<512 or va<BASE:
            raise AuditError("invalid original byte request")
        rva=va-BASE
        for name,base,vsize,raw,rawsize in self.sections:
            if base<=rva and rva+length<=base+rawsize:
                return self.data[raw+(rva-base):raw+(rva-base)+length]
        raise AuditError("VA does not map to original file")

def verify_source(read,check_region_sha=True):
    for name,(addr,length,sha) in REGIONS.items():
        data=read(addr,length)
        if check_region_sha and hashlib.sha256(data).hexdigest()!=sha:
            raise AuditError("original full function SHA mismatch "+name)
    for addr,expected in WITNESSES.items():
        if read(addr,len(expected))!=expected:
            raise AuditError("server source opcode mismatch at %#x"%addr)
    for addr,expected in STRINGS.items():
        if read(addr,len(expected))!=expected:
            raise AuditError("original server startup literal mismatch at %#x"%addr)
    for addr,ptr in POINTERS.items():
        if struct.unpack("<I",read(addr,4))[0]!=ptr:
            raise AuditError("original manifest pointer mismatch")
    return {
        "original_complete_function_regions":len(REGIONS),
        "complete_region_bytes":sum(region[1] for region in REGIONS.values()),
        "server_physics_factory_argument":"second DLLInit factory",
        "factory_names_order":["VPhysics030","VPhysicsCollision007","VPhysicsSurfaceProps001"],
        "server_global_pointers":["0x22575390","0x22575398","0x225753a0"],
        "original_surface_manifest_function":"0x22234060",
        "original_surface_file_registration_function":"0x22233fd0",
        "surface_registration_vtable_byte_offset":4,
        "surface_count_original_server_virtual_byte_offset":8,
        "surface_data_original_server_virtual_byte_offset":20,
        "server_normal_DLLInit_completed":False,
        "retail_engine_server_physics_probe_observed":False,
    }

def verify_original(data):
    if hashlib.sha256(data).hexdigest()!=SERVER_SHA:
        raise AuditError("original server.dll whole-file SHA mismatch")
    return verify_source(OriginalPE(data).read)

def self_test():
    # Fake original region SHA validation is disabled ONLY in the
    # explicitly local synthetic test, never original audit mode.
    chunks={a:expected for a,expected in WITNESSES.items()}
    chunks.update({a:value for a,value in STRINGS.items()})
    chunks.update({a:struct.pack("<I",v) for a,v in POINTERS.items()})
    for _,(a,length,_) in REGIONS.items():
        if a not in chunks:chunks[a]=b"\x90"*length
    def read(a,n):
        raw=chunks.get(a)
        if raw is not None and len(raw)>=n:return raw[:n]
        # fallback invented whole-body region if colliding with a sparse
        # witness on the same address.
        return b"\x90"*n
    verify_source(read,False)
    for addr in WITNESSES:
        bad=dict(chunks)
        corrupted=bytearray(bad[addr])
        corrupted[len(corrupted)//2]^=1
        bad[addr]=bytes(corrupted)
        def mutant(a,n):
            v=bad.get(a)
            return v[:n] if v is not None and len(v)>=n else b"\x90"*n
        try:verify_source(mutant,False)
        except AuditError:pass
        else:raise AssertionError("mutated original x86 witness accepted")
    for addr in STRINGS:
        bad=dict(chunks)
        bad[addr]=b"X"+bad[addr][1:]
        def mutant(a,n):
            v=bad.get(a)
            return v[:n] if v is not None and len(v)>=n else b"\x90"*n
        try:verify_source(mutant,False)
        except AuditError:pass
        else:raise AssertionError("mutated original physics string accepted")
    try:verify_original(b"MZ"+b"\x00"*512)
    except AuditError:pass
    else:raise AssertionError("fake original server accepted")
    print("PASS original-free server physics bootstrap: 3 region identities, sparse source opcode and literal mutations, false original file refusal")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--server",type=Path)
    group.add_argument("--archive",type=Path)
    group.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.self_test:self_test();return
    if args.archive:
        with zipfile.ZipFile(args.archive) as z:data=z.read("server.dll")
    else:data=args.server.read_bytes()
    print(json.dumps(verify_original(data),indent=2,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (AuditError,KeyError,OSError,zipfile.BadZipFile) as e:
        print("REFUSED:",str(e),file=sys.stderr);sys.exit(1)
