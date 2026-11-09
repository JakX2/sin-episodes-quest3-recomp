#!/usr/bin/env python3
"""SHA-pinned original SiN (2006) CStatAdvisor evaluator decomp audit.

Read-only owner files. Chains the existing Phase81 original *both-DLL*
verification; never emits proprietary bytes or original implementation code.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_advisor_state_flow import audit as prior_phase81_audit
from audit_sin_manager_state import PEImage, VerificationError

# Original machine-code regions: (start VA, bytes, full SHA256).
REGIONS = {
    "slot9": (0x222C4D00, 55, "ba4446916b2baada2f5229979341353f023ebaa439771cd95f00718d6dda4aa6"),
    "slot10": (0x222C4D40, 54, "26fd8d85153ce0f15d8f330a47c6e578def1ca78f5523e50e1dd480eee14417d"),
    "slot11": (0x222C4D80, 12, "f81dfaa3b110b49a0b389c8a6ac3e33e6c57a7740464ed32763d6062e9c66bd1"),
    "gate": (0x222C4D90, 34, "87727a576cb0f316ce7ce278056bb560756a83d6b36b4703fadca42a34986250"),
    "gate_callback": (0x222C4DC0, 66, "1ae1d1ce3a05553c94fa47aad02c2d8dd4bb0c1082b43d0bdef9f89ec50a4372"),
}
VTABLE = {
    0x2245CBF8: 0x222C4D00,  # CStatAdvisor slot 9
    0x2245CBFC: 0x222C4D40,  # CStatAdvisor slot 10
    0x2245CC00: 0x222C4D80,  # CStatAdvisor slot 11
}
OPCODES = {
    0x222C4D00:"d9412c", 0x222C4D03:"d85928",
    0x222C4D06:"d9411c", 0x222C4D0B:"d85930",
    0x222C4D0E:"f6c405", 0x222C4D13:"7a11",
    0x222C4D15:"f6c405", 0x222C4D18:"7b11",
    0x222C4D1A:"c7412003000000", 0x222C4D26:"f6c441",
    0x222C4D29:"75ef", 0x222C4D2B:"c7412004000000",
    0x222C4D40:"d94128", 0x222C4D43:"d8592c",
    0x222C4D46:"d9411c", 0x222C4D4B:"d85924",
    0x222C4D4E:"f6c441", 0x222C4D53:"7510",
    0x222C4D55:"f6c441", 0x222C4D58:"7410",
    0x222C4D5A:"b801000000", 0x222C4D5F:"894120",
    0x222C4D65:"f6c405", 0x222C4D68:"7af0",
    0x222C4D6A:"c7412000000000",
    0x222C4D80:"c7412002000000",
    0x222C4D87:"32c0",
    0x222C4D90:"d94134", 0x222C4D93:"d85c2404",
    0x222C4D99:"f6c441", 0x222C4D9C:"7505",
    0x222C4DA3:"d9442404", 0x222C4DA9:"d84138",
    0x222C4DAC:"d95934",
    0x222C4DC3:"d94634", 0x222C4DC6:"d85c2408",
    0x222C4DCC:"f6c441", 0x222C4DCF:"742b",
    0x222C4DE9:"ff5204", 0x222C4DEC:"d95e1c",
}
CALLS = {0x222C4DE0:0x222C6750, 0x222C4DF1:0x222C4C80}

def verify_original_evaluators(pe, check_hashes=True):
    for site, target in VTABLE.items():
        if pe.at(site,4) != struct.pack("<I",target):
            raise VerificationError("CStatAdvisor vtable mismatch at %#x" % site)
    if pe.at(0x2245CBD0,4) != struct.pack("<I",0x224CA970):
        raise VerificationError("CStatAdvisor RTTI locator mismatch")
    if pe.at(0x22521924,19) != b".?AVCStatAdvisor@@\x00":
        raise VerificationError("CStatAdvisor RTTI class-name mismatch")
    for site, text in OPCODES.items():
        raw = bytes.fromhex(text)
        if pe.at(site,len(raw)) != raw:
            raise VerificationError("CStatAdvisor original opcode mismatch at %#x" % site)
    for site,target in CALLS.items():
        expected = b"\xe8" + struct.pack("<i",target-site-5)
        if pe.at(site,5) != expected:
            raise VerificationError("CStatAdvisor original CALL mismatch at %#x" % site)
    if check_hashes:
        for name,(start,size,sha) in REGIONS.items():
            if hashlib.sha256(pe.at(start,size)).hexdigest() != sha:
                raise VerificationError("CStatAdvisor original SHA-256 mismatch in " + name)
    return {
        "original_class":"CStatAdvisor",
        "vtable_va":"0x2245CBD4",
        "slot9":{"entry":"0x222C4D00","codes":[3,4],"returns_low_byte":1},
        "slot10":{"entry":"0x222C4D40","codes":[0,1],"returns_low_byte":1},
        "slot11":{"entry":"0x222C4D80","codes":[2],"returns_low_byte":0},
        "gate":{"entry":"0x222C4D90","members":["+0x34","+0x38"]},
        "gate_callback":{"entry":"0x222C4DC0","status":"call path traced; not implemented"},
        "verified_vtable_pointers":len(VTABLE),
        "verified_instruction_sites":len(OPCODES),
        "verified_direct_calls":len(CALLS),
        "sha_pinned_code_regions":len(REGIONS) if check_hashes else 0,
        "scope":"Original virtual classification, not original class/method ABI, engine integration or full difficulty gameplay",
    }

def audit(client_bytes,server_bytes):
    result = prior_phase81_audit(client_bytes,server_bytes)  # checks both exact DLL SHA fingerprints
    result["cstatadvisor_evaluators"] = verify_original_evaluators(PEImage(server_bytes))
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    args=p.parse_args()
    result=audit(args.client.read_bytes(),args.server.read_bytes())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(result["cstatadvisor_evaluators"],indent=2))

if __name__=="__main__":
    main()
