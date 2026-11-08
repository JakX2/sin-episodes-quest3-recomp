#!/usr/bin/env python3
"""Read-only SHA-pinned 2006 SiN server.dll world-transform witnesses.

The owner's unchanged original x86 PE32 server.dll is required to audit.
Original bytes, disassembly and game assets are never emitted or embedded.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
  'rebuild_world_transform':(0x2213FA60,0x2213FBFC,'2d1b490fb47de7268785aa821b4db1580c6acb307b8a4ba22e8980cce66bbf6f'),
  'angle_origin_to_matrix':(0x2226CC20,0x2226CC4A,'7cb35b38f51f106e70930bf8f3637477c0c6e2990f37c3a966ad895613e04ce3'),
  'affine_matrix_concatenation':(0x22269520,0x222696B2,'968bed368c5b8c76fc04c0c4e3216a63e9e500dc3e9ca494a8c8cb6e49d46422'),
  'matrix_to_euler_angles':(0x22268A60,0x22268B1E,'b5ce81403027ee8943ddd895fc754f45649e42f562dbad0cd598300891ffacb3'),
  'select_parent_attachment_matrix':(0x2213D960,0x2213DA26,'96dddd569f3d00481aabc67ae38803dab4259c442e9bcbf5eb74efb7d949dac6'),
  'copy_matrix_3x4':(0x22268EF0,0x22268F04,'eda2988761a27e6b7a0f9a1fe41d482da5c805a3a6bc23ee934769a34c83be6e'),
  'extract_matrix_translation':(0x22268FD0,0x22268FF2,'58664c1352836a7208d74ca836930df3ed935777016c91f9f7d0c031ab3b6a47'),
  'compare_angles_vector':(0x2213A300,0x2213A35B,'d635c714f204dd99a7c1acddb6dff7a184c3619fe619a781e16c7b0adcd30124'),
}
CALLS={
  0x2213FACF:0x2226CC20,
  0x2213FB68:0x2213D960,
  0x2213FB6E:0x22269520,
  0x2213FB79:0x22268EF0,
  0x2213FB88:0x22268FD0,
  0x2213FB97:0x2213A300,
  0x2213FBAD:0x2202F0B0,
  0x2213FBD6:0x22268A60,
  0x2213FBE2:0x2221DFA0,
  0x2213FBEC:0x22221DB0,
  0x2213D9F1:0x2226CC20,
  0x2213DA13:0x2213FA60,
}
ANCHORS={
  0x2213FA66:'8b86b8000000',  # +0xb8 cached state
  0x2213FA71:'f6c101',       # check bit 0x800
  0x2213FA7A:'80e4f7',       # clear bit 0x800
  0x2213FA7F:'8dae60030000', # local origin +0x360
  0x2213FA88:'8dbe6c030000', # local angles +0x36c
  0x2213FAB6:'8d9e68020000', # world matrix +0x268
  0x2213FAD4:'8b86f0000000', # parent EHANDLE +0xf0
  0x2213FAEA:'81e1ff0f0000', # entity lookup entry 12-bit mask
  0x2213FAF7:'c1e80c',       # EHANDLE generation index
  0x2213FB12:'8986bc020000', # abs origin +0x2bc
  0x2213FB44:'898654030000', # abs angles +0x354
  0x2213FB90:'68c4855822',   # zero local angle reference
  0x2213FBA0:'80beec00000000', # parent attachment selector +0xec
  0x2213FBDE:'8bce6a04',     # transformation callback mask 4
}

def verify(data:bytes):
    digest=hashlib.sha256(data).hexdigest()
    if digest!=SERVER_SHA:
        raise ValueError('original 2006 server.dll SHA-256 mismatch')
    pe=PE32(data)
    if pe.base!=0x22000000:
        raise ValueError('original PE32 preferred image base mismatch')
    for name,(start,end,sha) in FUNCTIONS.items():
        if hashlib.sha256(pe.read(start,end-start)).hexdigest()!=sha:
            raise ValueError('original complete function differs: '+name)
    for at,target in CALLS.items():
        if pe.branch_target(at,0xE8)!=target:
            raise ValueError('original CALL target differs: '+hex(at))
    for at,encoding in ANCHORS.items():
        if pe.read(at,len(encoding)//2)!=bytes.fromhex(encoding):
            raise ValueError('instruction witness differs: '+hex(at))
    return {'original_server_sha256':digest,'functions_verified':len(FUNCTIONS),
            'call_edges_verified':len(CALLS),
            'instruction_anchors_verified':len(ANCHORS),
            'world_matrix_offset':'0x268','world_origin_offset':'0x2BC',
            'world_angles_offset':'0x354','local_origin_offset':'0x360',
            'local_angles_offset':'0x36C','parent_handle_offset':'0xF0',
            'attachment_selector_offset':'0xEC',
            'original_binary_included':False}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    args=p.parse_args()
    try:
        print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (ValueError,OSError) as exc:
        p.exit(1,'FAIL: '+str(exc)+'\n')
if __name__=='__main__':main()
