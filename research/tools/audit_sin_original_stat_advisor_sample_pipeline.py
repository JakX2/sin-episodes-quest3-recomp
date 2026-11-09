#!/usr/bin/env python3
"""Original SiN 2006 CStatAdvisor sample/update/dispatch machine-code auditor.

Owner-only original DLL checks. Does not publish code bytes, game assets,
engine binaries or native ABI guesses. Phase 149 chains Phase 148 and its
prior original SHA-verified client+server audit.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_stat_advisor_evaluators import audit as phase148_audit
from audit_sin_manager_state import PEImage, VerificationError

REGION_HASHES = {
    "six_advisor_batch": (0x222BAE20, 60, "8bc6209a82366141a09ef8b2cf3094917f172670bf2780b2ec0a8b19fcc86f6b"),
    "route_decision": (0x222C4C80, 126, "0b40f3c8e6aeb436b3dd8cc1bebbe4aebb92f50484ab956a73b3496d38f20c46"),
    "sample_callback": (0x222C4DC0, 66, "1ae1d1ce3a05553c94fa47aad02c2d8dd4bb0c1082b43d0bdef9f89ec50a4372"),
    "sample_fanout": (0x222C6750, 42, "7ccb2679f144b8b485ce2638b524e208aef6ef93c7b35615d935b7eb8071a1ee"),
    "sample_average": (0x222C6830, 72, "c89963bb26fa2e9f3a4ca307d25ee916fe9f0750ed132b620948b9165ea72497"),
}
# MSVC class RTTI/vtable found in Phase 81/148. Slot 1 (aggregation) was
# previously unanalyzed. The last three entries are the Phase 148 classifiers.
VTABLE = {
    0x2245CBD8: 0x222C6830,
    0x2245CBF8: 0x222C4D00,
    0x2245CBFC: 0x222C4D40,
    0x2245CC00: 0x222C4D80,
}
# Exact byte witnesses, excluding large original code excerpts.
# These validate owner field accesses, loops, x87 status tests, virtual
# indices, and sample callback ordering. Complete hashes cover every byte.
OPCODES = {
    0x222BAE28: "8d7548", 0x222BAE2B: "bf06000000",
    0x222BAE30: "8b442414", 0x222BAE34: "8b0e",
    0x222BAE3C: "84c0", 0x222BAE3E: "7402",
    0x222BAE40: "b301", 0x222BAE42: "83c604",
    0x222BAE45: "83ef01", 0x222BAE48: "75e6",
    0x222BAE4A: "84db", 0x222BAE4C: "7407",
    0x222BAE4E: "8bcd", 0x222BAE59: "c20400",
    0x222C4C80: "80793c00",        # +0x3C state byte
    0x222C4C86: "8b511c",          # current score +0x1C
    0x222C4C8C: "ff502c",          # neutral virtual slot11
    0x222C4C92: "d94128", 0x222C4C95: "d8592c",
    0x222C4C98: "d9411c", 0x222C4C9D: "d85928",
    0x222C4CA0: "f6c441", 0x222C4CA5: "7529",
    0x222C4CA7: "f6c441", 0x222C4CAA: "7429",
    0x222C4CAC: "d9412c", 0x222C4CAF: "d85928",
    0x222C4CB2: "d9411c", 0x222C4CB7: "d8592c",
    0x222C4CBA: "f6c405", 0x222C4CBF: "7a26",
    0x222C4CC1: "f6c405", 0x222C4CC4: "7b26",
    0x222C4CCC: "ff502c", 0x222C4CD0: "f6c405",
    0x222C4CD3: "7ad7", 0x222C4CDA: "d86128",
    0x222C4CDE: "d9e1", 0x222C4CE0: "d91c24",
    0x222C4CE3: "ff5028", 0x222C4CE7: "f6c441",
    0x222C4CEA: "75da", 0x222C4CF1: "d8611c",
    0x222C4CF5: "d9e1", 0x222C4CFA: "ff5224",
    0x222C4DC3: "d94634", 0x222C4DC6: "d85c2408",
    0x222C4DCC: "f6c441", 0x222C4DCF: "742b",
    0x222C4DD1: "d9442408", 0x222C4DD9: "d84638",
    0x222C4DDD: "d95e34", 0x222C4DE9: "ff5204",
    0x222C4DEC: "d95e1c", 0x222C4DF6: "b001",
    0x222C6756: "397714", 0x222C675C: "8b5c2410",
    0x222C6760: "8b4708", 0x222C6769: "ff520c",
    0x222C676C: "83c601", 0x222C676F: "3b7714",
    0x222C6838: "8b5f14", 0x222C6847: "895c2410",
    0x222C6858: "ff5204", 0x222C685B: "d844240c",
    0x222C6864: "d95c240c", 0x222C686A: "db442410",
    0x222C6871: "d83c24", # x87 FDIVR sum/count
}
CALLS = {0x222BAE37: 0x222C4DC0,
         0x222BAE50: 0x222BA6A0,
         0x222C4DE0: 0x222C6750,
         0x222C4DF1: 0x222C4C80}

def verify_sample_pipeline(server_pe, check_hashes=True):
    for va, target in VTABLE.items():
        if server_pe.at(va, 4) != struct.pack('<I', target):
            raise VerificationError('CStatAdvisor vtable mismatch at %#x' % va)
    for va, text in OPCODES.items():
        raw = bytes.fromhex(text)
        if server_pe.at(va, len(raw)) != raw:
            raise VerificationError('CStatAdvisor sample opcode mismatch at %#x' % va)
    for va, target in CALLS.items():
        expected = b'\xe8' + struct.pack('<i', target - va - 5)
        if server_pe.at(va, 5) != expected:
            raise VerificationError('CStatAdvisor sample CALL mismatch at %#x' % va)
    if check_hashes:
        for name, (va, size, sha) in REGION_HASHES.items():
            if hashlib.sha256(server_pe.at(va, size)).hexdigest() != sha:
                raise VerificationError('CStatAdvisor SHA-256 mismatch for ' + name)
    return {
        'six_advisor_batch_va': '0x222BAE20',
        'conditional_scores_sender_va': '0x222BA6A0',
        'sample_callback_va': '0x222C4DC0',
        'sample_gate_offsets': ['+0x34', '+0x38'],
        'listener_fanout_va': '0x222C6750',
        'listener_pointer_array_offset': '+0x08',
        'listener_count_offset': '+0x14',
        'listener_submit_virtual_slot': 3,
        'aggregate_virtual_slot': 1,
        'aggregate_function_va': '0x222C6830',
        'listener_value_virtual_slot': 1,
        'aggregate_operation': 'sum of listener float returns / signed count',
        'score_store_offset': '+0x1C',
        'route_dispatch_va': '0x222C4C80',
        'route_gate_offset': '+0x3C',
        'route_virtual_slots': [9, 10, 11],
        'verified_code_regions': len(REGION_HASHES) if check_hashes else 0,
        'verified_opcode_sites': len(OPCODES),
        'verified_call_edges': len(CALLS),
        'limits': 'Synthetic model/ABI-neutral: no engine module or exact x87 numeric bit parity claimed',
    }

def audit(client_bytes, server_bytes):
    out = phase148_audit(client_bytes, server_bytes)  # full SHA pinned, both DLLs
    out['stat_advisor_sample_pipeline'] = verify_sample_pipeline(PEImage(server_bytes))
    return out

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server', required=True, type=Path)
    p.add_argument('--client', required=True, type=Path)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    result = audit(args.client.read_bytes(), args.server.read_bytes())
    output = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output, encoding='utf-8')
    else:
        print(output, end='')

if __name__ == '__main__':
    main()
