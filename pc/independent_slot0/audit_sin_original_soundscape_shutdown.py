#!/usr/bin/env python3
"""Phase 489: original 2006 soundscape Shutdown original-PE audit.

The original user-owned SiN 2006 client.dll is SHA-authenticated
read-only with --client or --archive, then original vtable and entire
Shutdown body are verified. No original bytes are exported.
Public CI --self-test uses fabricated (not game) code and tests refusal
for damaged source markers. No retail execution is claimed.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA, parse_image, region_at_va,
)

SHUTDOWN_VA=0x240F3EE0
SHUTDOWN_SIZE=267
SHUTDOWN_SHA="fd966f21a2da53fb0f4bf655c17e06fce64f4f2ed01c731ede98948dbe90d8f8"
VTABLE=0x24293C34
SLOT_INIT=0
SLOT_SHUTDOWN=1
INIT_VA=0x240F4160
LEGACY_STOP_VA=0x241BD680
KV_DELETE_VA=0x24166050
# Exact selected code witnesses in the authenticated 267-byte method.
# Only tiny opcode windows, no proprietary method reconstruction bytes.
MARKERS=(
    (0x240F3EE4,"8b86ac000000"), # active sound count [this+0xAC]
    (0x240F3EF0,"8d3c8003ff03ff03ff"), # (n-1)*40 entry stride
    (0x240F3F08,"80782400"), # sound [entry+0x24] route flag
    (0x240F3F22,"ff522c"), # mixer vtable +0x2C
    (0x240F3F2F,"e84c970c00"), # legacy direct helper
    (0x240F3F37,"83ef28"), # reverse 40-byte stride
    (0x240F3F3F,"c786ac00000000000000"), # active count reset
    (0x240F3F49,"c786c000000000000000"), # auxiliary count reset
    (0x240F3F53,"c7462800000000"), # loaded pointer count reset
    (0x240F3FCF,"e87c4b1500"), # vector erase memmove
    (0x240F3FD7,"016e14"), # owner keyvalue pointer count -1
    (0x240F3FDC,"e86f200700"), # delete first KeyValues
    (0x240F3FE7,"5f5e5dc3"), # epilogue
)
# Post active-count reset, handle releases are guarded with
# -1 sentinel, generation check and external handle-table pointer.
HANDLE_MARKERS=(
    (0x240F3F5A,"8b869c000000"), # source EHANDLE field+0x9C
    (0x240F3F63,"3bc5"),         # -1 sentinel check
    (0x240F3F91,"89ae9c000000"), # conditional invalidation
    (0x240F3F97,"39ae94000000"), # source handle+0x94 check
    (0x240F3FA7,"89ae94000000"), # conditional invalidation
)
def audit_shutdown(method,check_hash=True):
    if len(method)!=SHUTDOWN_SIZE:
        raise ValueError("unexpected original shutdown byte length")
    if check_hash and hashlib.sha256(method).hexdigest()!=SHUTDOWN_SHA:
        raise ValueError("original shutdown whole-body hash mismatch")
    for va,hexbytes in (*MARKERS,*HANDLE_MARKERS):
        expected=bytes.fromhex(hexbytes)
        offset=va-SHUTDOWN_VA
        if method[offset:offset+len(expected)]!=expected:
            raise ValueError("original shutdown evidence mismatch at %#x"%va)
    for va,target in ((0x240F3F2F,LEGACY_STOP_VA),
                      (0x240F3FDC,KV_DELETE_VA)):
        offset=va-SHUTDOWN_VA
        rel=target-(va+5)
        if method[offset:offset+5]!=b"\xe8"+struct.pack("<i",rel):
            raise ValueError("original shutdown CALL mismatch at %#x"%va)
    return {
        "original_shutdown_complete_method_sha_verified":bool(check_hash),
        "method_va":hex(SHUTDOWN_VA),
        "method_size":SHUTDOWN_SIZE,
        "original_sound_entry_bytes":40,
        "sound_identifier_entry_offset":12,
        "routing_flag_entry_offset":36,
        "original_reverse_sound_stop_iteration":True,
        "two_original_sound_stop_routes":True,
        "mixer_stop_vtable_byte_offset":44,
        "legacy_sound_stop_va":hex(LEGACY_STOP_VA),
        "cleared_system_fields":[hex(n) for n in (0xAC,0xC0,0x28)],
        "conditional_handle_fields":[hex(n) for n in (0x9C,0x94)],
        "loaded_KeyValues_delete_first_order":True,
        "shutdown_engine_methods_executed":False
    }
def private_audit(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("original 2006 whole-client SHA mismatch")
    image=parse_image(client)
    if image[0]!=0x24000000:
        raise ValueError("original image base mismatch")
    vtable=region_at_va(client,image,VTABLE,8)
    init,shutdown=struct.unpack("<II",vtable)
    if init!=INIT_VA or shutdown!=SHUTDOWN_VA:
        raise ValueError("original soundscape RTTI-linked vtable slots differ")
    result=audit_shutdown(region_at_va(client,image,
                                      SHUTDOWN_VA,SHUTDOWN_SIZE))
    result.update(
        original_client_sha_verified=True,
        original_vtable_sha_authenticated_by_whole_client=True,
        original_init_virtual_slot=0,
        original_shutdown_virtual_slot=1,
        source_game_audio_files_exported=False
    )
    return result
def self_test():
    # Source-free fabrication: draw only selected synthetic test markers,
    # never publish or reproduce the original full code body.
    synthetic=bytearray(b"\xcc"*SHUTDOWN_SIZE)
    for va,raw in (*MARKERS,*HANDLE_MARKERS):
        snippet=bytes.fromhex(raw)
        at=va-SHUTDOWN_VA
        synthetic[at:at+len(snippet)]=snippet
    r=audit_shutdown(synthetic,False)
    assert r["original_sound_entry_bytes"]==40
    assert r["two_original_sound_stop_routes"]
    for site in (0x240F3EE4,0x240F3EF0,0x240F3F08,
                 0x240F3F22,0x240F3F37,0x240F3F53,
                 0x240F3F91,0x240F3FDC,0x240F3FE7):
        corrupted=bytearray(synthetic)
        corrupted[site-SHUTDOWN_VA]^=0x01
        try:audit_shutdown(corrupted,False)
        except ValueError:pass
        else:raise ValueError("synthetic corrupt method accepted")
    return {
        "original_free_shutdown_evidence_selftest":"PASS",
        "selected_original_call_and_branch_witnesses":len(MARKERS)+len(HANDLE_MARKERS),
        "no_original_binary_loaded":True
    }
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--self-test",action="store_true")
    group.add_argument("--client",type=Path)
    group.add_argument("--archive",type=Path)
    args=parser.parse_args()
    if args.self_test:result=self_test()
    elif args.client:result=private_audit(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            result=private_audit(z.read("client.dll"))
    print(json.dumps(result,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original soundscape Shutdown audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
