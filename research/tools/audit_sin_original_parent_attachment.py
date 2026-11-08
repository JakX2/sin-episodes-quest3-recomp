#!/usr/bin/env python3
"""Read-only SHA-pinned audit of original 2006 SiN server.dll attachment handling.

No original copyrighted DLL bytes, code or raw disassembly are embedded.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
    'parent_matrix_selector':(0x2213D960,0x2213DA26,
       '96dddd569f3d00481aabc67ae38803dab4259c442e9bcbf5eb74efb7d949dac6'),
    'attachment_pose_getter':(0x220ED470,0x220ED528,
       '9e494cdb180a40125d0f2c729c8bbce48ed5bfd31453c9d17d1417a2d690a200'),
    'identity_matrix_writer':(0x22269F60,0x22269F7F,
       '8a69616b88c12596c8fcf630020ade7c23d892a14600c58d51f368246bd5c757'),
    'angles_and_origin_matrix':(0x2226CC20,0x2226CC4A,
       '7cb35b38f51f106e70930bf8f3637477c0c6e2990f37c3a966ad895613e04ce3'),
}
CALLS={
    0x2213D99A:0x22269F60,
    0x2213D9D9:0x220ED470,
    0x2213D9F1:0x2226CC20,
    0x2213DA13:0x2213FA60,
    0x220ED498:0x22268A60,
    0x220ED4A9:0x22268FD0,
    0x220ED4CA:0x2213FA60,
    0x220ED4FC:0x2213FA60,
    0x2213FB68:0x2213D960,
}
ANCHORS={
    0x2213D967:'8b87f0000000',
    0x2213D97A:'81e1ff0f0000',
    0x2213D987:'c1e80c',
    0x2213D98A:'394104',
    0x2213D9AC:'80bfec00000000',
    0x2213D9B9:'ff90c4000000',
    0x2213D9D1:'8d54240c',
    0x2213D9DE:'84c0',
    0x2213DA03:'8b8eb8000000',
    0x2213DA09:'c1e90b',
    0x2213DA19:'8d8668020000',
    0x220ED484:'ff90c4020000',
    0x220ED4B1:'b001',
    0x220ED51F:'32c0',
    0x22269F67:'b90c000000',
    0x22269F70:'b80000803f',
}
def verify(data):
    digest=hashlib.sha256(data).hexdigest()
    if digest!=SERVER_SHA:raise ValueError('original 2006 server.dll SHA mismatch')
    pe=PE32(data)
    if pe.base!=0x22000000:raise ValueError('wrong image base')
    for name,(first,end,expected) in FUNCTIONS.items():
        if hashlib.sha256(pe.read(first,end-first)).hexdigest()!=expected:
            raise ValueError('function witness mismatch: '+name)
    for address,target in CALLS.items():
        if pe.branch_target(address,0xE8)!=target:
            raise ValueError('call witness mismatch at '+hex(address))
    for address,hexbytes in ANCHORS.items():
        if pe.read(address,len(hexbytes)//2)!=bytes.fromhex(hexbytes):
            raise ValueError('opcode witness mismatch at '+hex(address))
    return {'original_server_sha256':digest,
            'complete_function_hashes':len(FUNCTIONS),
            'relative_calls':len(CALLS),
            'opcode_anchors':len(ANCHORS),
            'attachment_selector_offset':'0xEC',
            'parent_handle_offset':'0xF0',
            'parent_world_matrix_offset':'0x268',
            'parent_vtable_animating_offset':'0xC4',
            'attachment_virtual_offset':'0x2C4',
            'original_binary_included':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as ex:p.exit(1,'FAIL: '+str(ex)+'\n')
if __name__=='__main__':main()
