#!/usr/bin/env python3
"""Read-only, owner-private audit of SiN 2006 module startup ABI and queries.

The full native Init bodies are authenticated by whole-method SHA256.
The source model implements only the precisely bounded acquisition prefix.
--self-test uses invented instruction bytes, never proprietary binary data.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_original_createinterface_phase657 import PE, u32

DEFAULT_MANIFEST = Path(__file__).resolve().parents[1] / "data" / "original_module_init_phase658.json"

def va(text: str) -> int:
    return int(text, 16)

def rel32_end(site: int, code: bytes, disp_at: int, count: int) -> int:
    return (site + count + struct.unpack_from("<i", code, disp_at)[0]) & 0xffffffff

def check_query_bytes(code: bytes, q: dict, target: int) -> None:
    site = va(q["site_va"])
    literal = va(q["name_string_va"])
    dest = va(q["storage_va"])
    if q.get("first_client_special"):
        if (len(code) != 30 or code[:3] != b"\x6a\x00\x68" or
            u32(code, 3) != literal or code[7] != 0xa3 or
            u32(code, 8) != 0x242fa458 or code[12:14] != b"\xff\xd6" or
            code[14:17] != b"\x83\xc4\x28" or
            code[17:19] != b"\x85\xc0" or code[19] != 0xa3 or
            u32(code, 20) != dest or code[24:26] != b"\x0f\x84" or
            rel32_end(site, code, 26, 30) != target):
            raise ValueError(f"original first-client interface query mismatch at {site:#x}")
        return
    call = b"\xff\xd7" if q["factory"] == "filesystem" else b"\xff\xd6"
    if (len(code) != 25 or code[:3] != b"\x6a\x00\x68" or
        u32(code, 3) != literal or code[7:9] != call or
        code[9:12] != b"\x83\xc4\x08" or
        code[12:14] != b"\x85\xc0" or code[14] != 0xa3 or
        u32(code, 15) != dest or code[19:21] != b"\x0f\x84" or
        rel32_end(site, code, 21, 25) != target):
        raise ValueError(f"original required interface query mismatch at {site:#x}")

def audit_module(kind: str, pe: PE, cfg: dict, expected_sha: str) -> dict:
    image_base = {"server":0x22000000, "client":0x24000000}[kind]
    if pe.base != image_base or hashlib.sha256(pe.data).hexdigest() != expected_sha:
        raise ValueError(f"{kind} entire 2006 retail DLL SHA256 mismatch")
    start, size, fail = va(cfg["entry_va"]), int(cfg["bytes"]), va(cfg["fail_exit_va"])
    body = pe.code(start, size)
    if hashlib.sha256(body).hexdigest() != cfg["sha256"]:
        raise ValueError(f"{kind} complete original Init body mismatch")
    ret = b"\xc2\x10\x00" if kind == "server" else b"\xc2\x0c\x00"
    if not body.endswith(ret) or pe.code(start + size, 1) != b"\xcc":
        raise ValueError(f"{kind} original RET and INT3 boundary mismatch")
    expected_count = 16 if kind=="server" else 21
    if len(cfg["queries"]) != expected_count:
        raise ValueError(f"{kind} expected {expected_count} required factory queries")
    if kind == "server" and (body[:9] != b"\x83\xec\x0c\x56\x8b\x74\x24\x14\x57" or
       pe.code(0x221e7743,4) != b"\x8b\x7c\x24\x20"):
        raise ValueError("server first engine and third filesystem factory ABI mismatch")
    if kind == "client" and (body[:6] != b"\x56\xe8\x2a\x37\x02\x00" or
       pe.code(0x240fe101,8) != b"\x8b\x44\x24\x30\x8b\x74\x24\x28"):
        raise ValueError("client original first factory and global-vars ABI mismatch")
    previous = start
    for q in cfg["queries"]:
        at = va(q["site_va"])
        if not (previous <= at and at < start + size):
            raise ValueError(f"{kind} interface queries not in monotonic original order")
        previous = at + (30 if q.get("first_client_special") else 25)
        if pe.asciiz(va(q["name_string_va"])) != q["version"]:
            raise ValueError(f"{kind} original requested name mismatch")
        pe.belongs(va(q["storage_va"]), ".data")
        check_query_bytes(pe.code(at, 30 if q.get("first_client_special") else 25), q, fail)
    # The interface acquisition prefix ends with the mandatory emitter
    # vtable+0 call, which receives the engine factory and checks AL.
    if kind == "server":
        if (pe.code(0x221e77dd,21) !=
           b"\x8b\x0d\xd0\x21\x57\x22\x8b\x01\x56\xff\x10\x84\xc0\x0f\x84\xb1\x01\x00\x00\x8b\x4c"):
            # Full method SHA covers this site already; check instruction
            # boundaries separately below to avoid a looser semantic claim.
            if pe.code(0x221e77e5,5) != b"\x56\xff\x10\x84\xc0":
                raise ValueError("server original sound emitter init gate mismatch")
        if pe.code(0x221e7942,7) != b"\x6a\x00\x68\xb8\x28\x44\x22":
            raise ValueError("server optional debug overlay query missing")
        if pe.code(0x221e7949,2) != b"\xff\xd6" or \
           pe.code(0x221e7950,5) != b"\xa3\xcc\x21\x57\x22":
            raise ValueError("server debug overlay storage mismatch")
        if pe.asciiz(va(cfg["deferred_optional_debug_overlay"]["string_va"])) != "VDebugOverlay003":
            raise ValueError("server debug overlay literal mismatch")
    else:
        if pe.code(0x240fe31b,9) != b"\x8b\x10\x56\x8b\xc8\xff\x12\x84\xc0":
            raise ValueError("client original sound emitter init gate mismatch")
        if pe.code(0x240fe347,9) != b"\x6a\x00\x68\x34\x69\x29\x24\xff\xd6":
            raise ValueError("client optional material stub query mismatch")
        if pe.asciiz(va(cfg["optional_material_stub"]["string_va"])) != "VMaterialSystemStub001":
            raise ValueError("client optional material stub literal mismatch")
        if pe.code(0x240fe35b,8) != b"\x6a\x00\xa3\xa0\xf6\x33\x24\x8b":
            raise ValueError("client optional stub storage mismatch")
        if pe.code(0x240fe362,12)[:8] != b"\x68\x14\x69\x29\x24\xff\x50\x20":
            raise ValueError("client hardware config queried through wrong virtual call")
        if pe.asciiz(va(cfg["required_hardware_config"]["string_va"])) != "MaterialSystemHardwareConfig012":
            raise ValueError("client hardware configuration interface name mismatch")
        if pe.code(0x240fe36a,13) != b"\x85\xc0\xa3\xa4\xf6\x33\x24\x0f\x84\x27\x02\x00\x00":
            raise ValueError("client required material config NULL gate mismatch")
    return {"original_va":cfg["entry_va"],"original_bytes":size,
            "required_factory_queries":len(cfg["queries"]),
            "factory_routes":{which:sum(q["factory"]==which for q in cfg["queries"])
                              for which in ("engine","filesystem")}}

def audit(manifest:dict, server:bytes, client:bytes)->dict:
    return {
        "server":audit_module("server",PE(server),manifest["functions"]["server"],
                              manifest["retail_sha256"]["server"]),
        "client":audit_module("client",PE(client),manifest["functions"]["client"],
                              manifest["retail_sha256"]["client"]),
        "authentication":"owner-retail exact method bodies; no original runtime execution",
    }

def self_test()->None:
    fake_site=0x10004000
    name=0x10009000
    dest=0x1000f000
    target=0x10005000
    fragment=(b"\x6a\x00\x68"+struct.pack("<I",name)+
              b"\xff\xd6\x83\xc4\x08\x85\xc0\xa3"+struct.pack("<I",dest)+
              b"\x0f\x84"+struct.pack("<i",target-(fake_site+25)))
    q={"site_va":hex(fake_site),"name_string_va":hex(name),"storage_va":hex(dest),
       "version":"synthetic", "factory":"engine"}
    check_query_bytes(fragment,q,target)
    for at in (3,7,10,14,15,19,21):
        mutated=bytearray(fragment);mutated[at]^=1
        try:check_query_bytes(bytes(mutated),q,target)
        except ValueError:pass
        else:raise AssertionError(f"fake site mutation at +{at} accepted")
    fs=dict(q,factory="filesystem")
    try:check_query_bytes(fragment,fs,target)
    except ValueError:pass
    else:raise AssertionError("filesystem/engine factory provenance was ignored")
    first=(b"\x6a\x00\x68"+struct.pack("<I",name)+b"\xa3"+
           struct.pack("<I",0x242fa458)+b"\xff\xd6\x83\xc4\x28\x85\xc0\xa3"+
           struct.pack("<I",dest)+b"\x0f\x84"+struct.pack("<i",target-(fake_site+30)))
    check_query_bytes(first,dict(q,first_client_special=True),target)
    bogus=bytearray(first);bogus[8]^=1
    try:check_query_bytes(bytes(bogus),dict(q,first_client_special=True),target)
    except ValueError:pass
    else:raise AssertionError("client global-vars ABI mutation accepted")
    print("Phase658 invented-byte acquisition-site mutation self-test PASS")

def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--server",type=Path)
    ap.add_argument("--client",type=Path)
    ap.add_argument("--manifest",type=Path,default=DEFAULT_MANIFEST)
    ap.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:self_test();return
    if not args.server or not args.client:
        ap.error("provide --server and --client owner-original DLL paths, or --self-test")
    manifest=json.loads(args.manifest.read_text(encoding="utf-8"))
    result=audit(manifest,args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":main()
