#!/usr/bin/env python3
"""Evidence audit of original 2006 SiN CGrenadeAR detonation and think/touch.

Read-only, original-server-SHA-pinned, PE32 address checks. The output is
metadata; it does not include original game code or pretend that original
virtual function addresses are callable on ARM64. Distinguishes observed
machine instructions from unresolved Source engine interface semantics.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

DETONATE_VA = 0x22314570
THINK_VA = 0x22314360
TOUCH_VA = 0x223142F0
VIRTUAL_DETONATE_BYTE_OFFSET = 0x3E8
# Each verified direct CALL rel32 in the three original game functions.
CALLS = {
    0x2231461F: 0x22286BF0,  # referenced entity cleanup
    0x2231464C: 0x222701C0,  # initialize effect/trace context
    0x22314664: 0x22270950,  # prepare context around projectile origin
    0x223147F5: 0x22014CC0,  # trace path
    0x2231483D: 0x2228B320,  # decal dispatch
    0x22314873: 0x22288A90,  # shared area camera/feedback routine, role uncertain
    0x223148E7: 0x222B2730,  # owner/attacker handle resolver
    0x223148F5: 0x221F89C0,  # construct damage-info-like state, name provisional
    0x223148FB: 0x222A5E10,  # shared engine virtual effect/damage dispatch
    0x22314901: 0x22286BF0,  # final projectile cleanup
    0x2231437A: 0x2214D7B0,  # next-think scheduler
    0x22314559: 0x221B8FA0,  # script/sound dispatcher from think path
}
# Original x86 instructions tied to control flow or member layout.
WITNESSES = {
    0x22314579: bytes.fromhex("80be9507000000"),    # flag +0x795 == 0
    0x223145A5: bytes.fromhex("c6869507000000"),    # clear flag +0x795
    0x2231466F: bytes.fromhex("8b86ac070000"),      # field +0x7AC
    0x22314675: bytes.fromhex("8b8e98070000"),      # field +0x798
    0x22314743: b"\xd8\x0d" + struct.pack("<I", 0x223DDDCC), # fmul 60
    0x22314817: b"\x68" + struct.pack("<I", 0x22407CB0), # Scorch
    0x22314833: b"\x68" + struct.pack("<I", 0x22477BD4), # SmallScorch
    0x2231485E: b"\x68" + struct.pack("<f", 750.0),
    0x22314863: b"\x68" + struct.pack("<f", 1.0),
    0x22314868: b"\x68" + struct.pack("<f", 150.0),
    0x2231486D: b"\x68" + struct.pack("<f", 25.0),
    0x2231487E: bytes.fromhex("8b9698070000"),      # field +0x798
    0x2231489F: bytes.fromhex("83f8ff"),            # compare invalid entity handle
    0x2231433D: bytes.fromhex("80bf9507000000"),     # touch flag check
    0x2231434A: bytes.fromhex("ff92e8030000"),       # call vtable +0x3E8
    0x22314394: b"\xd8\x05" + struct.pack("<I",0x224771F8), # now+0.1 compare
    0x22314444: bytes.fromhex("ff90e8030000"),       # call vtable +0x3E8
    0x2231444A: bytes.fromhex("d986c4070000"),       # evolving field +0x7C4
    0x22314450: b"\xd8\x1d" + struct.pack("<I",0x224771FC), # vs 300.0
    0x22314463: b"\xdc\x05" + struct.pack("<I",0x22467448), # +15.0 double
    0x22288A95: b"\xd8\x1d" + struct.pack("<I",0x22455F38), # screen shake clamp
    0x22288AA2: bytes.fromhex("c744240c00008041"), # clamp amplitude to 16
    0x222A5E39: bytes.fromhex("ff90b8000000"), # global engine vtable +0xB8
}
ORIGINAL_STRINGS = {
    0x22407CB0: b"Scorch\x00",
    0x22477BD4: b"SmallScorch\x00",
}
FLOATS = {
    0x223D9CB0: 0.05, # repeated think schedule
    0x224771F8: 0.1,  # think-time compare
    0x223DDDCC: 60.0, # vector displacement multiplier during trace
    0x224771FC: 300.0, # evolving +0x7C4 threshold
    0x22455F38: 16.0, # actual UTIL_ScreenShake clamp in original SiN server
}
DOUBLES = {0x22467448: 15.0}

def check_byte_witnesses(pe: PEImage) -> None:
    for addr, original in WITNESSES.items():
        if pe.at(addr, len(original)) != original:
            raise VerificationError(f"detonation instruction mismatch at 0x{addr:08X}")
    for addr, text in ORIGINAL_STRINGS.items():
        if pe.at(addr, len(text)) != text:
            raise VerificationError(f"original decal literal mismatch at 0x{addr:08X}")
    for addr, value in FLOATS.items():
        actual = struct.unpack("<f", pe.at(addr, 4))[0]
        if struct.pack("<f", actual) != struct.pack("<f", value):
            raise VerificationError(f"float evidence mismatch at 0x{addr:08X}")
    for addr, value in DOUBLES.items():
        if struct.unpack("<d", pe.at(addr, 8))[0] != value:
            raise VerificationError(f"double evidence mismatch at 0x{addr:08X}")

def verify_direct_calls(pe: PEImage) -> list[dict]:
    edges = []
    for site, expected in sorted(CALLS.items()):
        op = pe.at(site, 5)
        if op[0] != 0xE8:
            raise VerificationError(f"missing x86 CALL at 0x{site:08X}")
        actual = site + 5 + struct.unpack_from("<i", op, 1)[0]
        if actual != expected:
            raise VerificationError(f"wrong original call target at 0x{site:08X}")
        edges.append({"callsite_va":f"0x{site:08X}",
                      "target_va":f"0x{actual:08X}"})
    return edges

def audit(server: bytes, *, check_hash: bool=True) -> dict:
    digest = hashlib.sha256(server).hexdigest()
    if check_hash and digest != SERVER_SHA256:
        raise VerificationError("original 2006 SiN server.dll SHA-256 mismatch")
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("not the original SiN server PE image base")
    check_byte_witnesses(pe)
    edges = verify_direct_calls(pe)
    return {
        "source":"original 2006 SiN Episodes server.dll",
        "sha256":digest,
        "method":{
            "class":"CGrenadeAR",
            "identified_original_Detonate_va":f"0x{DETONATE_VA:08X}",
            "virtual_slot":250,
            "touch_callback_va":f"0x{TOUCH_VA:08X}",
            "think_callback_va":f"0x{THINK_VA:08X}",
            "virtual_dispatch_byte_offset":f"0x{VIRTUAL_DETONATE_BYTE_OFFSET:X}",
        },
        "state":{
            "detonation_guard_flag_offset":"0x795",
            "guard_behavior":"returns without effects if zero; clears on detonation",
            "explosion_parameters_read_offsets":["0x798","0x7AC"],
            "evolving_think_field_offset":"0x7C4",
            "think_requeue_seconds":struct.unpack("<f",pe.at(0x223D9CB0,4))[0],
            "think_time_comparison_delta_seconds":struct.unpack("<f",pe.at(0x224771F8,4))[0],
            "evolving_field_upper_comparison":300.0,
            "evolving_field_increment":15.0,
        },
        "effects":{
            "decals":["Scorch","SmallScorch"],
            "original_decal_dispatch_va":"0x2228B320",
            "trace_function_va":"0x22014CC0",
            "trace_vector_scale":60.0,
            "area_feedback_function_va":"0x22288A90",
            "feedback_call_literal_float_args":[750.0,1.0,150.0,25.0],
            "feedback_helper_original_amplitude_clamp":16.0,
            "engine_virtual_effect_call_offset":"0x4C",
            "damage_info_like_constructor_va":"0x221F89C0",
            "damage_effect_dispatch_va":"0x222A5E10",
            "damage_effect_dispatch_engine_virtual_offset":"0xB8",
            "final_remove_call_va":"0x22286BF0",
        },
        "verified_original_direct_calls":edges,
        "limitations":[
            "Label Detonate is based on four independently named grenade Detonate exports from prior report.",
            "Field +0x798 and +0x7AC enter effect/damage-related paths, but neither is yet formally identified as radius or damage in the original class.",
            "Camera feedback at 0x22288A90 and damage dispatch at 0x222A5E10 have contextual evidence; their exact Source API identifiers and argument types remain to prove.",
            "Literal effect parameter values are not automatically physical VR units.",
            "Other indirect/virtual calls and trace branch outcomes require full control-flow and original runtime comparison.",
            "This does not create an Android ARM64 gameplay module.",
        ],
    }

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    try:
        result = audit(args.server.read_bytes())
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    except (OSError, ValueError, struct.error) as error:
        p.exit(1, "SiN AR DETONATION AUDIT FAILED: " + str(error) + "\n")
    print(json.dumps({"verified_calls":len(result["verified_original_direct_calls"]),
                      "class":result["method"]["class"]}, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
