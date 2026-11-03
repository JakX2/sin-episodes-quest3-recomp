#!/usr/bin/env python3
"""Verify original SiN CStatAdvisor values, virtual propagation and message fields.

Chained Phase 80 SHA-pinned PE32 analysis requires BOTH owner-held 2006 DLLs.
Report contains decoded metadata only, never original proprietary code bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_challenge_advisor_presets import audit as phase80_audit
from audit_sin_manager_state import PEImage, VerificationError

OPCODES={
    # Original 0x222C0B90 dispatcher stack values passed to 0x222B9450.
    0x222c0bbd:'8b4c240c',0x222c0bc1:'8b542410',
    0x222c0bc5:'8b442414',0x222c0bc9:'51',
    0x222c0bca:'8b4c241c',0x222c0bce:'52',
    0x222c0bcf:'50',0x222c0bd0:'51',0x222c0bd1:'57',
    # Indexed manager advisor instance, middle sum and original FABS.
    0x222b9450:'8b442414',0x222b9454:'8b54240c',
    0x222b945a:'8b7c240c',0x222b946a:'8b4cbe48',
    0x222b9475:'d9442414',0x222b9479:'d8442418',
    0x222b947d:'8b4cbe48',0x222b9481:'8b11',
    0x222b9483:'51',0x222b9484:'d9e1',
    0x222b9486:'d80d78973d22',0x222b948c:'d91c24',
    0x222b948f:'ff5208',
    # Four float32 values at original advisor +0x24..+0x30.
    0x222c4ac0:'8b442404',0x222c4ac4:'8b542408',
    0x222c4ac8:'894130',0x222c4acb:'8b44240c',
    0x222c4acf:'89512c',0x222c4ad2:'8b542410',
    0x222c4ad6:'894128',0x222c4ad9:'895124',
    0x222c4adc:'c21000',
    # Virtual +8 scalar setter +0x1C; replay method in same vtable.
    0x222c4ab0:'8b442404',0x222c4ab4:'89411c',
    0x222c4a90:'8b511c',0x222c4a93:'8b01',
    0x222c4a95:'52',0x222c4a96:'ff5008',0x222c4a99:'c3',
    # Listeners at [object+0x08], count at +0x14, virtual +8.
    0x222c6722:'8bf9',0x222c6724:'33f6',
    0x222c6726:'397714',0x222c6730:'8b4708',
    0x222c6733:'8b0cb0',0x222c6736:'8b11',
    0x222c6738:'53',0x222c6739:'ff5208',
    0x222c673c:'83c601',0x222c673f:'3b7714',
    0x222c6742:'7cec',
    # Manager builder returns six pointers +0x48 through +0x5C.
    0x222b9bdf:'c7442408a0ca4522',
    0x222b9c0c:'894644',0x222b9c13:'894e48',
    0x222b9c1a:'89564c',0x222b9c21:'894650',
    0x222b9c29:'894e54',0x222b9c2c:'895658',
    0x222b9c2f:'89465c',
    0x222bc56f:'6a40',
    0x222bc598:'c700d4cb4522',
    0x222bc5a5:'c7402002000000',
    # Manager loops across six, separately storing auxiliary +0x34.
    0x222b9427:'8d7148',0x222b942a:'bf06000000',
    0x222b9430:'8b0e',0x222b9438:'83c604',
    0x222b943b:'83ef01',0x222b943e:'75f0',
    0x222c4aa0:'8b442404',0x222c4aa4:'894134',
    0x222c4aa7:'c20400',
    # Already independently identified outgoing usermessage reads.
    0x222ba703:'8b421c',0x222ba70e:'8b4920',
    0x222ba7e5:'8b4224',0x222ba7f0:'8b5128',
    0x222ba7fb:'8b482c',0x222ba806:'8b4230',
}
DIRECT_CALLS={
    0x222c0bd9:0x222b9450,
    0x222b9470:0x222c4ac0,
    0x222b9bff:0x222bc520,
    0x222b9433:0x222c4aa0,
}
DIRECT_JUMPS={0x222c4ab7:0x222c6720}
VTABLE_ENTRIES={
    0x2245cbd0:0x224ca970,  # MSVC complete object locator
    0x224ca97c:0x2252191c, # RTTI type descriptor
    0x2245cbd4:0x222bbea0,
    0x2245cbdc:0x222c4ab0,
    0x2245cbf4:0x222c4a90,
}
REGION_HASHES={
    (0x222b9450,71):'759e2673b79c55b2b32d7cf50f07c95b1916b71500a4d5d7efe47615f9528eac',
    (0x222c4ac0,31):'8eb6d0e71fe33671092e6014d3becd81ee9cc5f55ed7932ec565516209fc7cd9',
    (0x222c4ab0,12):'6f60510c1b0e8207f98f76e4885eccc2ce8f72d23782defa3b6dd0a39c3394fe',
    (0x222c6720,40):'cf52e14ca2137852bef319b2ccb9a1a9d7fefa07e6a784326ba5de001e4860a8',
    (0x222c4a90,10):'ded93bdec07f205b31e0bf83cf6f47ee2e7ddcafd2a17c8e5b768f4a981258bb',
    (0x222c4aa0,10):'6d731f768f8834c3595c479205dc6991ea8b4b940958f6be330f60aa9373eef1',
    (0x222b9bb0,135):'5d609b5bc72a8062457ae46028cd3a15c7447d9987cfc2ba447c63ba37c5dc0d',
    (0x2245cbd4,48):'efdb55171447c9d310aaba5d662333c80a4c21cbcf6a64255aa61f62cc4164f2',
}
def check(pe,va,expected):
    if pe.at(va,len(expected))!=expected:
        raise VerificationError('Phase 81 CStatAdvisor x86 mismatch at %#x'%va)

def verify_advisor_state_flow(server_pe,check_hashes=True):
    for va,raw in OPCODES.items():
        check(server_pe,va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        check(server_pe,va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in DIRECT_JUMPS.items():
        check(server_pe,va,b'\xe9'+struct.pack('<i',target-va-5))
    for va,target in VTABLE_ENTRIES.items():
        check(server_pe,va,struct.pack('<I',target))
    check(server_pe,0x223d9778,struct.pack('<f',0.5))
    check(server_pe,0x22521924,b'.?AVCStatAdvisor@@\x00')
    if check_hashes:
        for (va,size),digest in REGION_HASHES.items():
            if hashlib.sha256(server_pe.at(va,size)).hexdigest()!=digest:
                raise VerificationError('Phase 81 SHA-256 region mismatch at %#x'%va)
    return {
        'rtti_class':'CStatAdvisor',
        'manager_six_pointer_offsets':['0x48','0x4C','0x50','0x54','0x58','0x5C'],
        'initializer_va':'0x222C0B90',
        'per_advisor_wrapper_va':'0x222B9450',
        'four_float_setter_va':'0x222C4AC0',
        'four_float_member_offsets':['0x24','0x28','0x2C','0x30'],
        'score_member_offset':'0x1C',
        'finite_score_equation':'0.5 * abs(channel[1] + channel[2])',
        'score_virtual_setter_va':'0x222C4AB0',
        'score_notification_fanout_va':'0x222C6720',
        'listener_array_offset':'0x08',
        'listener_count_offset':'0x14',
        'listener_virtual_slot_offset':'0x08',
        'replay_value_va':'0x222C4A90',
        'auxiliary_broadcast_va':'0x222B9420',
        'auxiliary_setter_va':'0x222C4AA0',
        'auxiliary_member_offset':'0x34',
        'scores_message_float_source_va':'0x222BA703',
        'scores_message_code_source_va':'0x222BA70E',
        'channels_message_float_sources':['0x222BA7E5','0x222BA7F0','0x222BA7FB','0x222BA806'],
        'opcode_sites':len(OPCODES),'call_edges':len(DIRECT_CALLS),
        'jump_edges':len(DIRECT_JUMPS),'vtable_rtti_edges':len(VTABLE_ENTRIES),
        'sha_regions':len(REGION_HASHES) if check_hashes else 0,
        'scope':'Initializer/field routing and event fanout, not original performance advisor formulas or full SourceVR ABI.'
    }

def audit(client,server):
    previous=phase80_audit(client,server) # both exact original 2006 DLL hashes
    previous['advisor_state_flow']=verify_advisor_state_flow(PEImage(server))
    return previous

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result['advisor_state_flow'],indent=2))
if __name__=='__main__':main()
