#!/usr/bin/env python3
"""SHA-pinned, read-only audit of the original SiN (2006) server.dll
CBaseAnimating attachment getters, metadata and inherited MSVC x86 vtables.
No proprietary binary bytes are copied into the repository or outputs.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
 'GetAttachmentMatrix':(0x220ED530,0x220ED630,'db55c501578dbbcdc70256310ae140ba26b90fc42fff5abb8c07b0f976fb28ef'),
 'GetBoneTransform':(0x220ED1B0,0x220ED230,'4fea7931f33e8d0825a422c77b46fe46653b049ceea1e09344f7a7c1bd4e1165'),
 'StudioAttachmentCount':(0x2211B810,0x2211B829,'c01d329074c66276f1ed40723629dd134f6a37473fe371c5818358914c7dca9e'),
 'StudioAttachmentRecord':(0x2211BFA0,0x2211BFF4,'e1c2733c852a330bc95bc8eb4bed05c0e9293872cf6165790dec37e1491e8ac1'),
 'StudioAttachmentBoneIndex':(0x2211C000,0x2211C04F,'d8dd507bd7db4278fab89175b4c04228e8de13fcead4e21d973cdbff243263f0'),
 'MatrixGetColumn':(0x22268FD0,0x22268FF2,'58664c1352836a7208d74ca836930df3ed935777016c91f9f7d0c031ab3b6a47'),
 'MatrixSetColumn':(0x22269000,0x22269022,'0cca1ab3a3d09af77ed64dd0ea321862c2b5d3eaaeb4fcf269207299b531dd4b'),
 'PoseQueryCaller':(0x220ED470,0x220ED528,'9e494cdb180a40125d0f2c729c8bbce48ed5bfd31453c9d17d1417a2d690a200'),
}
CALLS={
 0x220ED537:0x22139A10,0x220ED57F:0x2211B810,
 0x220ED593:0x2211BFA0,0x220ED59D:0x2211C000,
 0x220ED5C7:0x22269520,0x220ED5E6:0x22268FD0,
 0x220ED5FA:0x22268B20,0x220ED604:0x22269F60,
 0x220ED611:0x22269000,0x220ED1B4:0x22139A10,
 0x220ED1F2:0x220EB8D0,0x220ED1FA:0x221004C0,
 0x220ED20A:0x220EBE70,0x220ED210:0x22268EF0,
 0x220ED223:0x22268EF0,
}
ANCHORS={
 0x220ED574:'83ff01',           # index must be >=1
 0x220ED586:'0f8f',             # reject index > numattachments
 0x220ED58D:'83c7ff',           # one-based index -> zero-based
 0x220ED5AD:'ff92b0020000',   # bone virtual +0x2B0
 0x220ED5B3:'f6430601',       # +6 flags bit 0x100
 0x220ED5B9:'8b542460',       # output 3x4 on ordinary route
 0x220ED5D2:'b001',           # success
 0x220ED5E0:'6a03',           # attachment translation col 3
 0x220ED5F0:'8d442438',       # world-aligned intermediate matrix
 0x220ED625:'5f5e32c0',       # invalid index -> false
 0x220ED484:'ff90c4020000',   # caller +0x2C4
 0x2211B810:'83b95001000000', # studio hdr cache offset +0x150
 0x2211BFE6:'8b81f4000000',   # attachment records offset +0xf4
 0x2211C028:'8b4f08',         # attachment bone index +8
 0x220ED1E8:'3bb09c000000',   # bone bounds
}
VTABLES={
 'CBaseAnimating':(0x223FE6E4,0x224B9590,0x224DC1A0),
 'CBaseAnimatingOverlay':(0x223FF6AC,0x224B9798,0x224DC1C0),
 'CBaseFlex':(0x223FF994,0x224B97DC,0x224DC1E4),
 'CBaseCombatCharacter':(0x22459E5C,0x224CA2BC,0x224DC1FC),
 'CBasePlayer':(0x2245222C,0x224C8498,0x224E5E60),
 'CHL2_Player':(0x224AEC3C,0x224D3FF0,0x2251E69C),
 'CAI_BaseNPC':(0x223DA7AC,0x224B6E1C,0x224DBF58),
 'CNPC_Jessica':(0x22497A14,0x224D224C,0x22535B0C),
}
def read(pe,va,n):
    fn=getattr(pe,'read',None)
    return (fn if callable(fn) else pe.read_va)(va,n)
def image_base(pe):
    return pe.base if hasattr(pe,'base') else pe.image_base
def branch_target(pe,va,op=0xE8):
    if hasattr(pe,'branch_target'):
        return pe.branch_target(va,op)
    b=read(pe,va,5)
    if b[0]!=op:raise ValueError('relative opcode mismatch')
    return va+5+struct.unpack('<i',b[1:])[0]
def ptr(pe,va):return struct.unpack('<I',read(pe,va,4))[0]
def verify(data):
    sha=hashlib.sha256(data).hexdigest()
    if sha!=SERVER_SHA:raise ValueError('original SiN 2006 server.dll digest mismatch')
    pe=PE32(data)
    if image_base(pe)!=0x22000000:raise ValueError('original image base mismatch')
    for name,(lo,hi,digest) in FUNCTIONS.items():
        if hashlib.sha256(read(pe,lo,hi-lo)).hexdigest()!=digest:
            raise ValueError('function witness mismatch: '+name)
    for at,target in CALLS.items():
        if branch_target(pe,at)!=target:
            raise ValueError('call target mismatch: '+hex(at))
    for at,payload in ANCHORS.items():
        b=bytes.fromhex(payload)
        if read(pe,at,len(b))!=b:
            raise ValueError('instruction witness mismatch: '+hex(at))
    for name,(table,col,descriptor) in VTABLES.items():
        if ptr(pe,table-4)!=col or ptr(pe,col+12)!=descriptor:
            raise ValueError('MSVC RTTI locator mismatch: '+name)
        decorated=('.?AV'+name+'@@\x00').encode('ascii')
        if read(pe,descriptor+8,len(decorated))!=decorated:
            raise ValueError('MSVC RTTI type name mismatch: '+name)
        if ptr(pe,table+0x2C4)!=0x220ED530:
            raise ValueError('inherited attachment slot mismatch: '+name)
        if ptr(pe,table+0x2B0)!=0x220ED1B0:
            raise ValueError('inherited bone slot mismatch: '+name)
    return {'server_sha256':sha,'full_function_hashes':len(FUNCTIONS),
            'call_targets':len(CALLS),'instruction_anchors':len(ANCHORS),
            'rtti_vtables':list(VTABLES),
            'virtual_attachment_slot':'0x2C4','virtual_bone_slot':'0x2B0',
            'studio_attachment_record_stride':92,
            'attachment_index':'1-based','world_align_flag':'0x100',
            'original_binary_included':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path,required=True)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as e:p.exit(1,'FAIL: '+str(e)+'\n')
if __name__=='__main__':main()
