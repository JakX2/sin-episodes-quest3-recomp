#!/usr/bin/env python3
"""Check actual SourceVR ARM64 game startup vtable dispatch at engine callsites.

PRIVATE, SHA-pinned SourceVR 0.5.5 archive in, JSON metadata out.
Checks 7 engine variants, real factory-owned client/server vtables and
their startup method prologues. Does NOT invoke any proprietary methods.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sourcevr_native_game_factories import (
    AuditError, NativeELF, PROFILE_SHAS, decode_add, decode_adrp, require,
)
from audit_sourcevr_engine_module_loader import ENGINE_WITNESSES
from audit_sourcevr_game_module_vtables import method_table, verify_module

# (client slot0 Init dispatch beginning at LDR x8,[x0],
#  client slot2 cleanup dispatch beginning at LDR x8,[x0],
#  server slot12 description dispatch beginning at LDR x8,[x0]).
# These are exact ELF VAs from the OWNER'S original Android native engine.
DISPATCH_VAS = {
    "hl2":         (0x328228, 0x328294, 0x2f3b1c),
    "ep1":         (0x328228, 0x328294, 0x2f3b1c),
    "ep2":         (0x328228, 0x328294, 0x2f3b1c),
    "lostcoast":   (0x328228, 0x328294, 0x2f3b1c),
    "portal":      (0x327a08, 0x327a74, 0x2f32fc),
    "portal2":     (0x32b848, 0x32b8b4, 0x2f713c),
    "entropyzero": (0x328228, 0x328294, 0x2f3b1c),
}


def expect(elf, va, raw, reason):
    require(elf.word(va) == raw, f"{reason} changed at {va:#x}")


def check_dispatch(elf, client_init, client_cleanup, server_description):
    # Engine passes native client singleton in x0, loads method 0 at +0x00.
    # x1 is a factory-like pointer, x2 copies x1, x3 is loaded from global.
    # The result in w0 controls a success/failure branch.
    init_ops = (
        0xf9400008,  # LDR x8,[x0]       game client's vptr
        0xf9400108,  # LDR x8,[x8]       vtable method 0
        None,        # LDR x3,[x3,#N]   global pointer; profile-specific
        0xaa0103e2,  # MOV x2,x1         second factory argument
        0xd63f0100,  # BLR x8            game client Init
        0x35000080,  # CBNZ w0,+0x10     uses bool/int result
    )
    for slot, expected in enumerate(init_ops):
        va = client_init + slot*4
        word = elf.word(va)
        if expected is not None:
            expect(elf,va,expected,"client game slot0 dispatch")
        else:
            require(word & 0xffc003ff == (0xf9400000 | (3<<5) | 3),
                    "native client Init x3 pointer argument load changed")
    expect(elf,client_init-4,0xf9400101,
           "client Init x1 factory pointer load")
    # The native vtable is read from the same x0 already loaded from
    # the engine's client object global a few instructions earlier.
    global_obj = elf.word(client_init-16)
    require(global_obj & 0xffc0001f == (0xf9400000 | 0),
            "client Init x0 object load no longer LDR")
    # Cleanup is an independently observed slot2 engine virtual call.
    for idx,w in enumerate((0xf9400008,0xf9400908,0xd63f0100)):
        expect(elf, client_cleanup+idx*4,w,
               "client game cleanup method slot2 dispatch")
    # Native engine passes server method12's x0 return in x2 to a
    # printf-like "server%s loaded for \"%s\"\n" message.
    for idx,w in enumerate(
        (0xf9400008,0xf9403108,0xd63f0100,0xaa0003e2)):
        expect(elf,server_description+idx*4,w,
               "server game description slot12 and string return")
    for reg, va, literal in (
        (0,server_description+16,b'server%s loaded for "%s"\n\x00'),
        (1,server_description+24,b".so\x00"),
    ):
        target=(decode_adrp(va,elf.word(va),reg) +
                decode_add(elf.word(va+4),reg))
        require(elf.at(target,len(literal))==literal,
                f"server description logging string changed at {va:#x}")
    return {
        "client_init_method_index": 0,
        "client_init_engine_dispatch_va": hex(client_init),
        "client_init_argument_registers": ["x0=this", "x1=pointer",
                                           "x2=pointer", "x3=pointer"],
        "client_init_result": "w0, checked with CBNZ",
        "client_cleanup_method_index": 2,
        "client_cleanup_engine_dispatch_va": hex(client_cleanup),
        "server_description_method_index": 12,
        "server_description_engine_dispatch_va": hex(server_description),
        "server_description_result": "x0 pointer used by native string logger",
    }


def check_actual_methods(client, server):
    _, cptrs, _, _ = method_table(client,"libclient.so")
    _, sptrs, _, _ = method_table(server,"libserver.so")
    cli_init,cli_cleanup = cptrs[0],cptrs[2]
    srv_init,srv_description = sptrs[0],sptrs[12]
    # Every one of the six client implementation variants has exactly
    # the same first method prologue consuming 3 pointer arguments.
    for off, value in (
        (0x00,0xd10143ff), (0x04,0xa9027bfd),
        (0x08,0xa90357f6), (0x0c,0xa9044ff4),
        (0x10,0x910083fd),
        (0x14,0xaa0303f4), # mov x20,x3: globals-like pointer
        (0x18,0xaa0203f3), # mov x19,x2: second factory pointer
        (0x1c,0xf81f83a1), # stur x1,[x29,#-8]: first factory pointer
    ):
        expect(client,cli_init+off,value,"native client method0 3-argument prologue")
    # Every one of five server variants similarly receives FOUR
    # pointer-sized arguments x1..x4. Engine's server slot0 callsite
    # remains to be found; no DLLInit callback type is asserted here.
    for off,value in (
        (0x00,0xd101c3ff), (0x04,0xa9027bfd),
        (0x08,0xf9001bf9), (0x0c,0xa9045ff8),
        (0x10,0xa90557f6), (0x14,0xa9064ff4),
        (0x1c,0xf9000fa1), # str x1,[x29,#0x18]
        (0x28,0xaa0403f5), # mov x21,x4
        (0x2c,0xaa0303f4), # mov x20,x3
        (0x30,0xaa0203f3), # mov x19,x2
    ):
        expect(server,srv_init+off,value,
               "native server method0 4-argument prologue")
    for off, value in ((0,0xa9bd7bfd),(4,0xf9000bf5),
                       (8,0xa9024ff4),(12,0x910003fd)):
        expect(client,cli_cleanup+off,value,
               "native client slot2 cleanup implementation")
    branch=server.word(srv_description)
    require(branch & 0xfc000000 == 0x14000000,
            "server game description must branch to its implementation")
    target=branch & 0x3ffffff
    if target & (1<<25): target -= 1<<26
    target=srv_description+4*target
    native_text=server.sections[".text"]
    require(native_text[3]<=target<native_text[3]+native_text[5],
            "server description implementation branch leaves code")
    return {
        "client_init_va":hex(cli_init),
        "client_cleanup_va":hex(cli_cleanup),
        "server_startup_method0_va":hex(srv_init),
        "server_description_va":hex(srv_description),
        "server_description_implementation_va":hex(target),
        "server_startup_register_arguments": ["x1","x2","x3","x4"],
        "startup_signature_types_fully_verified":False,
    }


def audit(archive_path):
    reports=[]
    with zipfile.ZipFile(archive_path) as z:
        for profile,(engine_sha,*_) in ENGINE_WITNESSES.items():
            manifest=z.read(
                f"source vr/assets/sourcevr/v1/native/{profile}.manifest"
            ).decode("utf-8")
            modules={}
            for row in manifest.splitlines():
                cells=row.split("\t")
                if (len(cells)>=5 and cells[2] in
                        ("libengine.so","libclient.so","libserver.so")):
                    kind=cells[2]
                    require(kind not in modules,f"{profile}: duplicate {kind}")
                    modules[kind]=(cells[1],"source vr/assets/"+cells[4])
            require(set(modules)=={"libengine.so","libclient.so","libserver.so"},
                    f"{profile}: missing native engine/client/server")
            require(modules["libengine.so"][0]==engine_sha and
                    (modules["libserver.so"][0],modules["libclient.so"][0])
                    ==PROFILE_SHAS[profile], "native profile SHA selection drift")
            blobs={}
            for kind,(sha,path) in modules.items():
                raw=z.read(path)
                require(hashlib.sha256(raw).hexdigest()==sha,
                        f"{profile}: {kind} SHA256 mismatch")
                blobs[kind]=NativeELF(raw)
            # Stronger than a string check: previously proven factory
            # callback, singleton, constructor vptr and all 47/78 methods.
            verify_module(blobs["libserver.so"],modules["libserver.so"][0])
            verify_module(blobs["libclient.so"],modules["libclient.so"][0])
            engine_calls=check_dispatch(blobs["libengine.so"],
                                        *DISPATCH_VAS[profile])
            implementations=check_actual_methods(
                blobs["libclient.so"],blobs["libserver.so"])
            reports.append({
                "profile":profile,"engine_sha256":engine_sha,
                "client_sha256":modules["libclient.so"][0],
                "server_sha256":modules["libserver.so"][0],
                **engine_calls,**implementations,
            })
    require(len(reports)==7,"native game profiles missing")
    return {
        "verified_profiles":reports,
        "finding":(
            "Actual native engine dispatches client vtable slot 0 with "
            "three pointer arguments and tests w0, dispatches client slot 2 "
            "during cleanup, and uses server slot 12 as a description-string "
            "provider. Real client/server method implementations and factory "
            "singleton/vtable chains are tied to full SHA-pinned game modules."
        ),
        "not_proven":[
            "exact C++ typed signatures for every native game interface method",
            "native engine server startup DLLInit callsite or its full argument types",
            "ability to return replacement 47/78-slot objects safely",
            "custom SourceVR game profile installation or SiN Quest 3 launch",
        ]
    }


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sourcevr-zip",required=True,type=Path)
    args=p.parse_args()
    try:
        print(json.dumps(audit(args.sourcevr_zip),indent=2))
    except (AuditError,KeyError,zipfile.BadZipFile,OSError,struct.error,
            UnicodeDecodeError) as exc:
        p.exit(2,f"Native game startup ABI audit FAILED: {exc}\n")

if __name__=="__main__":
    main()
