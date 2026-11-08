#!/usr/bin/env python3
"""Read-only original SiN 2006 server.dll pose-driver first-loop witnesses.
The owner supplies the private unchanged x86 DLL. No raw executable bytes
or disassembly are committed to the public GitHub repository.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32
ORIGINAL_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
 'EntirePoseDriver':(0x22103a60,0x221048d5,'14e05d53bbda8974eb74f26e9c4b089ca0593110ca64b45866fd8575b1afe0e0'),
 'ResetAndCommandLoopRegion':(0x22103a60,0x22103d65,'b51f7e36324409c2be2abebafeb90f67b82cab0fde3723f44bc656a650aa44f7'),
 'Type4And6HandlersRegion':(0x22103e3a,0x22103ee3,'72ba1b9faafa44adc22ef018773c84842fd1f2c7ad9ad89b736e0d61f5050514'),
 'QuaternionMixerWrapper':(0x2226d210,0x2226d245,'bace6f69a5ab185ecd48634c17860c4e1869fc4dd3a04b2f64c3cdbb99fefa4a'),
}
CALLS={
 0x22103b9a:0x2226d210, 0x22103dc8:0x22100f30,
 0x22103ddd:0x2226d250,0x22103ded:0x22268f10,
 0x22103e10:0x22269520,
 0x22103f2c:0x2226ca30,0x22104006:0x2226ad40,
 0x22104022:0x2226ae70,0x2210402f:0x2226acd0,
 0x22104148:0x2226ad40,0x221041c7:0x2226ad40,
 0x221041dd:0x2226ae70,0x221041ea:0x2226acd0,
 0x221042e5:0x22268fd0,0x22104301:0x22268fd0,
 0x22104320:0x22268fd0,
}
ANCHORS={
 0x22103a60:'81ec28020000', # original 0x228 stack
 0x22103a83:'897c085c', # track +0x5C cleared
 0x22103a8a:'b90000803f', # per-track value 1
 0x22103ab2:'8d4c080c', # reset position
 0x22103ad3:'05e4000000', # track stride 0xE4
 0x22103af0:'8b4518', # group array at object +0x18
 0x22103b0c:'8b4604', # command type +4
 0x22103b0f:'83c0fd', # type -3
 0x22103b1b:'ff2485d8481022', # four-entry dispatch table
 0x22103b22:'8b4610', # command track index +0x10
 0x22103b42:'837e0405', # distinguish type 5 payload
 0x22103b57:'817e580000803f', # alpha==1 exact comparison
 0x22103cb7:'8d562c', # raw quaternion +0x2C
 0x22103cd3:'8b4620', # raw position +0x20
 0x22103cf7:'d9465c', # direct cap component
 0x22103d0f:'837e0403', # type 3 special side effect
 0x22103d31:'834424107c', # command stride 0x7C
 0x22103d4e:'83c314', # group stride 0x14
 0x22103d5e:'397d0c', # start second loop
 0x22103e3a:'8b4610', # type6 track index
 0x22103e87:'8b4610', # type4 track index
 0x22103ecc:'d94658', # type4 alpha
 0x221048c8:'5f5e5d5b', # epilogue
}
JUMP_TABLE={3:0x22103b22,4:0x22103e87,5:0x22103b22,6:0x22103e3a}
CONSTANTS={0x223d2590:0.0,0x223d22f8:1.0}

def read(pe,va,n):
    fn=getattr(pe,'read',None)
    return fn(va,n) if callable(fn) else pe.read_va(va,n)
def base(pe):return pe.base if hasattr(pe,'base') else pe.image_base
def call_target(pe,va):
    b=read(pe,va,5)
    if b[0]!=0xe8:raise ValueError('not original E8 relative call at '+hex(va))
    return va+5+struct.unpack('<i',b[1:])[0]
def jump_entry(pe,kind):
    return struct.unpack('<I',read(pe,0x221048d8+(kind-3)*4,4))[0]
def verify(data):
    sha=hashlib.sha256(data).hexdigest()
    if sha!=ORIGINAL_SHA:raise ValueError('original server.dll SHA mismatch')
    pe=PE32(data)
    if base(pe)!=0x22000000:raise ValueError('unexpected PE image base')
    for name,(lo,hi,digest) in FUNCTIONS.items():
        if hashlib.sha256(read(pe,lo,hi-lo)).hexdigest()!=digest:
            raise ValueError('original function witness mismatch: '+name)
    for at,target in CALLS.items():
        if call_target(pe,at)!=target:raise ValueError('call mismatch '+hex(at))
    for at,encoded in ANCHORS.items():
        if read(pe,at,len(bytes.fromhex(encoded)))!=bytes.fromhex(encoded):
            raise ValueError('instruction mismatch '+hex(at))
    for kind,target in JUMP_TABLE.items():
        if jump_entry(pe,kind)!=target:raise ValueError('switch target mismatch '+str(kind))
    for at,want in CONSTANTS.items():
        if struct.unpack('<f',read(pe,at,4))[0]!=want:
            raise ValueError('numeric constant mismatch '+hex(at))
    return {'original_server_sha256':sha,'complete_region_hashes':len(FUNCTIONS),
      'verified_relative_calls':len(CALLS),'verified_instruction_anchors':len(ANCHORS),
      'type3_to_6_dispatch_targets':len(JUMP_TABLE),'command_stride_bytes':124,
      'track_stride_bytes':228,'group_stride_bytes':20,
      'covered_behavior':'reset and first command loop; subsequent pose evaluation excluded',
      'original_binary_included':False}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path,required=True)
    args=parser.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as exc:parser.exit(1,'FAIL: '+str(exc)+'\n')
if __name__=='__main__':main()
