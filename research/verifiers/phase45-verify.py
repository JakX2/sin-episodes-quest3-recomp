#!/usr/bin/env python3
"""Checks packaged Phase 44 *text excerpts*, not proprietary DLL bytes.
Use Phase 44's phase44_verify.py with original bin.zip for binary verification.
"""
from pathlib import Path
root = Path(__file__).parent / 'evidence'
a = (root / 'server_zoom_transition_conditions.asm').read_text().lower()
b = (root / 'server_zoom_fov_transitions.asm').read_text().lower()
c = (root / 'client_zoom_button_mask.asm').read_text().lower()
checks = {
    'client zoom bit mask': ('orl\t$0x80000, %esi' in c),
    'server player field low byte offset': ('0x93e(%esi)' in a),
    'server zoom bit test': ('testb\t$0x8, 0x93e(%esi)' in a),
    'server zoom flag': ('0x578(%edi)' in a),
    'server restore flag': ('0x579(%edi)' in a),
    'interrupt disable call': ('222ccd98:' in a and '0x222cbbc0' in a),
    'restoration enter call': ('222ccdb9:' in a and '0x222cbb60' in a),
    'zoom input branch': ('222ccdcb:' in a),
    'zoom animation vcall': ('*0x314(%eax)' in b),
    'enter activity argument 0xc0': ('pushl\t$0xc0' in b),
    'exit activity conditional': ('cmpl\t$0xc0, 0x4bc(%esi)' in b),
    'exit activity argument 0x9d': ('pushl\t$0x9d' in b),
    'zoom getter vcall': ('*0x440(%edx)' in b),
    'enter FOV duration 0.25': ('pushl\t$0x3e800000' in b),
    'exit FOV duration ~0.15': ('pushl\t$0x3e19999a' in b),
    'exit FOV sentinel': ('222cbbeb:' in b and 'pushl\t$0x0' in b),
}
for label, good in checks.items():
    print(('PASS' if good else 'FAIL'), label)
if not all(checks.values()):
    raise SystemExit(1)
print(f'PASS: {len(checks)} textual evidence checks; no original DLLs verified here')
