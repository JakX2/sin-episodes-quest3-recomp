#!/usr/bin/env python3
"""Phase1692: verify CGameStringPool against the owner-original PE32.
Only source metadata, no proprietary instruction dump or execution.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

CLIENT_SHA=ORIGINALS["client"]["sha256"]
TABLE_VA=0x2429AD94
TABLE_SHA="06ffa7ab0155f9aad47897f8b812e470f72074f98468d28b0df24cb44e254045"
TABLE=(
    0x240566C0,0x240566D0,0x240566E0,0x240CF700,
    0x240CF710,0x2413D2F0,0x24056700,0x24056710,
    0x24056720,0x24056730,0x240CF720,0x2413D300,
)
# Complete function spans (not including following INT3 padding).
METHODS=(
    (0x2413D240,6,"476c508b6290a607c69eb45a804d6a3321986ec976bb899174e13dd4c9bf4be9"),
    (0x240566C0,3,"e18aacce29affcdddaa5f641ac1ec12552e0fe9d066e6408941b1792818b8619"),
    (0x2413D250,24,"a36cd6ff0a653851123d66ea36f838c8604b6fd8ce85704acd3606b2ddc8f6d6"),
    (0x2413D270,16,"1a1173a891654fd688fd2c667cb139d24595b497af76c0b70b28b90acd0f8576"),
    (0x2413D2D0,21,"23a14d5c810719c406383b8ca599c7df48ef299157a5a1d2174e9ecbc3aad6dc"),
    (0x2413D2F0,8,"31299fceeaded3af4a5aa831a65d76867e53034a76eed26ec06d27658a673610"),
    (0x2413D300,46,"e661ba954a8b8e1cd5b39a5105b6c837db162845c3ae510f684bf8a8a49c919f"),
    (0x241C5EB0,32,"320c67c61dce759183366af9e33bb0fd81a8bf9d7084cdf0252a77ae7d1c22ee"),
    (0x241C6FB0,81,"75f36dd68ec1dbb04e8f19a2b491c4a4141195f6c9356c200cbf7bb8d4f68a5f"),
    (0x241C7010,56,"0898a3c7bd1e40fd7879e1e22106ea4f05970831d31b85b5810d15e70bce87c1"),
    (0x241C7050,66,"deecdce44984e7fb89a964fe31d1adaba923fb4391cb309596fd05601a3271b4"),
    (0x241C7110,46,"984577b928ef5f50ecd73089bd884ae9e93e1c7a2971bbb6de221a14fe03f6d4"),
    (0x241C71D0,85,"0a42b1ecfabd8da65296299c54e8f13ae37cf8ac51c1644b110fb52dd89e5815"),
    (0x241C6620,258,"8d27eeb0593caeb9c5981ce62b316a1be95fd39184adb6e6bdb6aa40a954bc27"),
    (0x241C6730,132,"4b88b3d84ce104b72e4916c208af022df2b341f3306c64c6fcf8a2ddb92c2ad6"),
    (0x241C7170,94,"0c3967b443e67448a62a06e50b6a8e690715e055e5316644dd6f8d65307a47f2"),
    (0x241C6300,86,"79b71853fe0e6f531af1438e85f88842f06137f4adcbb228dfe4814120c84ceb"),
    (0x241C70A0,109,"8952ad80edf4dafa02d3da778426a80b713b10b286c241cb96ae7eac42bc7ae9"),
    (0x24261CAE,43,"91bf3dc505406f7d831e3917e4bf668f8d7077e9486d9bc39fb95b09619acf68"),
    (0x2425F81B,67,"0905847aa7a5fa843447c965b7ed17f401401ad60bf6be835038df5637559806"),
    (0x24261C60,78,"9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658"),
)
# Independent source control-flow checks. 0xe8 CALL, 0xe9 JMP.
EDGES=(
    (0x240FE3F6,0x2413D240,0xe8),
    (0x240FE3FC,0x24156840,0xe8),
    (0x2413D25F,0x241C71D0,0xe8),
    (0x2413D27A,0x241C7010,0xe8),
    (0x2413D2D6,0x241C6FB0,0xe8),
    (0x2413D2F3,0x241C7050,0xe9),
    (0x2413D303,0x24156820,0xe8),
    (0x2413D313,0x241C7110,0xe8),
    (0x241C5EBE,0x2425F81B,0xe8),
    (0x2425F824,0x24261C60,0xe9),
    (0x241C701B,0x241C6620,0xe8),
    (0x241C71DB,0x241C6620,0xe8),
    (0x241C7202,0x24261CAE,0xe8),
    (0x241C7217,0x241C7170,0xe8),
    (0x241C7070,0x2416F940,0xe8),
    (0x241C708D,0x241C6730,0xe9),
    (0x241C7113,0x241C7050,0xe8),
    (0x241C7126,0x2416F940,0xe8),
)
def require(ok: bool, message: str) -> None:
    if not ok:
        raise VerificationError("Phase1692 CGameStringPool: "+message)

def verify_digest(raw: bytes, length: int, expected: str) -> None:
    require(len(raw)==length and hashlib.sha256(raw).hexdigest()==expected,
            "original complete byte range or digest mismatch")

def check_edge(raw: bytes, at: int, target: int, opcode: int) -> None:
    require(len(raw)==5 and raw[0]==opcode and
            at+5+struct.unpack_from("<i",raw,1)[0]==target,
            "source native CALL/JMP target mismatch at "+hex(at))

def verify_original(raw: bytes) -> dict:
    verify_digest(raw,len(raw),CLIENT_SHA)
    pe=PEImage(raw)
    require(pe.base==0x24000000,"original client PE32 image base drift")
    table=pe.at(TABLE_VA,48)
    verify_digest(table,48,TABLE_SHA)
    require(struct.unpack("<12I",table)==TABLE,"original twelve-slot table drift")
    for va,n,sha in METHODS:
        verify_digest(pe.at(va,n),n,sha)
        require(pe.section_of(va)==".text","source method not in original code")
    for at,target,op in EDGES:
        check_edge(pe.at(at,5),at,target,op)
    # Check key structural facts without reproducing commercial bodies.
    require(pe.at(0x2413D240,6)==b"\xb8"+struct.pack("<I",0x2436212C)+b"\xc3",
            "source first game system getter drift")
    require(pe.at(0x240566C0,3)==b"\xb0\x01\xc3","source Init not trivial true")
    require(pe.at(0x2413D254,5)==b"\x80\x38\x00\x74\x0c",
            "allocation wrapper no longer rejects empty C-string")
    require(pe.at(0x241C6FB9,5)==b"\x68\x00\x0c\x00\x00",
            "initial source tree allocation no longer 256*12")
    require(pe.at(0x241C6FC5,7)==b"\xc7\x46\x08\x00\x01\x00\x00" and
            pe.at(0x241C6FCC,7)==b"\xc7\x46\x0c\x20\x00\x00\x00",
            "source tree initial capacity or growth changed")
    require(pe.at(0x241C71E0,4)==b"\x66\x3d\xff\xff",
            "source intern invalid uint16 index changed")
    require(pe.at(0x241C705B,5)==b"\x66\x81\xfe\xff\xff",
            "source clear invalid uint16 index changed")
    return {"phase":1692,"whole_original_client_sha":CLIENT_SHA,
            "source_vtable_slots":len(TABLE),"source_complete_functions":len(METHODS),
            "source_call_edges":len(EDGES),"original_initial_nodes":256,
            "original_growth_nodes":32,"original_node_bytes":12,
            "original_missing_index":65535,
            "scope":"original static source proof; no execution, no original tree ABI implementation"}

def self_test() -> dict:
    invented=b"original-free digest fixture"
    digest=hashlib.sha256(invented).hexdigest()
    verify_digest(invented,len(invented),digest)
    rejected=0
    for i in range(len(invented)):
        bad=bytearray(invented);bad[i]^=1
        try: verify_digest(bytes(bad),len(invented),digest)
        except VerificationError: rejected+=1
        else: raise AssertionError("altered invented digest accepted")
    for opcode in (0xe8,0xe9):
        site=0x100000;target=0x100123
        raw=bytes((opcode,))+struct.pack("<i",target-site-5)
        check_edge(raw,site,target,opcode)
        for i in range(len(raw)):
            bad=bytearray(raw);bad[i]^=1
            try: check_edge(bytes(bad),site,target,opcode)
            except VerificationError: rejected+=1
            else: raise AssertionError("altered invented edge accepted")
    try: verify_original(b"not the owner DLL")
    except VerificationError: rejected+=1
    else: raise AssertionError("unmatched original accepted")
    return {"phase":1692,"invented_mutations_rejected":rejected,
            "original_executed":False,"original_verified":False}

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--self-test",action="store_true")
    source.add_argument("--archive",type=Path)
    source.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.self_test: result=self_test()
    elif a.archive:
        with zipfile.ZipFile(a.archive) as z:
            require(z.namelist().count("client.dll")==1,"archive missing exact client")
            result=verify_original(z.read("client.dll"))
    else: result=verify_original(a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
