#!/usr/bin/env python3
"""Original 2006 SiN class RTTI and vtable read-only verifier (both DLLs).

Originals are SHA256-pinned by chained Phase90 auditor. Outputs metadata only.
No copyrighted code, assets or x86 DLL bytes are redistributed.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_cache_records_client_shutdown import audit as previous_audit
from audit_sin_original_factories import PE32, VerificationError

CLIENT = [[607247504,607247508,606781196,607038416,".?AVCDebugOverlayPanel@@",[605998064,605997664],"87f2d851109e5f22ff5e36943e63474c732aebc75452b4ac74c14a27e7529937",4],[607248036,607248040,606784564,607039428,".?AVCNetGraphPanelInterface@@",[606012112,606009840],"abec7a6ef6d234509229a84768b2bf12cdcc75a22131f458b375551b00ddeec9",4],[607247616,607247620,606781936,607038632,".?AVCFPS@@",[605999792,605999520],"028594e92e9e343493a31e74977081701ed3f8cef2d6ff241b4a32cfec490bd1",4],[607247932,607247936,606784312,607039288,".?AVCMessageChars@@",[606006560,606005808,606006192,606006112,606006032,606005856,606006096,606006240,606006352],"4e063da9fc419500c7fc9c78dd4714bf26fa68cb5eadcb48285e0770040c3e60",4],[607247824,607247828,606783664,607039072,".?AVCLoadingDisc@@",[606004336,606004352,606004432,606004528],"d6e8219c5b5d5ae0cdacbbd5fa9ccbf6f88405f50bbb503ecfeb8674f27f5a04",4],[607247340,607247344,606780492,607038060,".?AVCCenterPrint@@",[605996080,605996160,605996112,605996240,605996208,605996272,605996896,605996304],"35451d0a78e637ba3a333d8f942768cf1a938954277b92ab226ab98fff92edcb",28],[607248192,607248196,606786692,607039804,".?AVCTextMessage@@",[606020832,606020384,606020432,606020464,606020496,606020544,606020576,606020608],"0b57722b26744b1f502242c9e0836ee28ad9d79a014498fbb5ccd3f8cfa1c007",4],[607252156,607252160,606796168,607042884,".?AVCStatsPanelInterface@@",[606109936,606107248,606108960,606109984,606107296],"07d490a513fdb6c1a14260fc32c9d10d14fd41efa0a605a89e791883a0841324",4]]
SERVER = [[574807200,575412256,".?AVCSceneCache@@",[572158816,572174496,572184400],"d0e88dc9b98044232979296247f0aa6916d6a3a9fa82817566d78f1a41b22e6e",572182480,28],[574663624,575388564,".?AVCModelSoundCache@@",[571750368,571757536,571757744],"694e23672e8a79cfd4f40b492de8f77193699fd0dd9e3e7d5572e4f437a64d70",571774832,24]]
SERVER_METHOD_ANCHORS = {"0x221a736a":"8b4704","0x221a7375":"668b6f14","0x221ab0a0":"81ec04020000","0x221ab0d6":"6800020000","0x221ad750":"83ec08","0x221437ea":"668b5f10","0x221453e0":"81ec04020000","0x22145410":"6800020000","0x221454b0":"8b542404","0x221454ed":"56ff5024","0x221454f1":"83f803"}

def u32(pe,address):
    return struct.unpack('<I',pe.at(address,4))[0]

def verify_table(pe,table,locator,name,methods,expected_hash):
    start,text=pe.code()
    if u32(pe,table-4)!=locator:
        raise VerificationError('RTTI locator mismatch')
    if pe.at(locator,12)!=bytes(12):
        raise VerificationError('MSVC x86 RTTI locator metadata mismatch')
    descriptor=u32(pe,locator+12)
    if pe.cstring(descriptor+8)!=name:
        raise VerificationError('RTTI type descriptor class mismatch')
    slots=pe.at(table,len(methods)*4)
    if hashlib.sha256(slots).hexdigest()!=expected_hash:
        raise VerificationError('original full-vtable SHA-256 mismatch')
    if list(struct.unpack('<%dI'%len(methods),slots))!=list(methods):
        raise VerificationError('original vtable function pointers differ')
    if any(not start<=m<start+len(text) for m in methods):
        raise VerificationError('original vtable method is not executable')
    if start<=u32(pe,table+len(methods)*4)<start+len(text):
        raise VerificationError('vtable slot count incomplete')
    return dict(decorated_original_name=name,class_name=name.removeprefix('.?AV').removesuffix('@@'),
                original_vtable=hex(table),original_RTTI=hex(locator),
                original_type_descriptor=hex(descriptor),
                verified_vtable_sha256=expected_hash,
                original_vtable_methods=[hex(m) for m in methods])

def inspect_client(pe):
    result=[]
    for i,(cell,obj,vt,locator,name,methods,digest,slot) in enumerate(CLIENT):
        if u32(pe,cell)!=obj or u32(pe,obj)!=vt:
            raise VerificationError('original client global/object vtable link mismatch')
        info=verify_table(pe,vt,locator,name,methods,digest)
        info.update(order=i+1,original_global_cell=hex(cell),
                    original_singleton_object=hex(obj),
                    shutdown_virtual_offset_bytes=slot,
                    actual_original_shutdown_method=hex(methods[slot//4]))
        result.append(info)
    return dict(client_shutdown_classes=result,class_count=8,
                note='RTTI and exact vtables verified; these are not native ARM64 method pointers')

def inspect_server(pe):
    result=[]
    for vt,locator,name,methods,digest,insert,allocation in SERVER:
        info=verify_table(pe,vt,locator,name,methods,digest)
        info.update(original_find_or_insert=hex(insert),
                    original_new_auxiliary_allocation_bytes=allocation,
                    source_method_slots=3)
        result.append(info)
    for key,op in SERVER_METHOD_ANCHORS.items():
        address=int(key,16)
        if pe.at(address,len(bytes.fromhex(op)))!=bytes.fromhex(op):
            raise VerificationError('original cache class method opcode mismatch at %#x'%address)
    return dict(server_cache_classes=result,class_count=2,
                verified_method_opcode_anchors=len(SERVER_METHOD_ANCHORS),
                note='Per-class methods identified, not complete on-disk serialization signatures')

def audit(client,server):
    previous_audit(client,server)
    return dict(client=inspect_client(PE32(client)),server=inspect_server(PE32(server)),
                limits=['not a playable Quest3 port','does not extract original DLL code or cached assets',
                        'original Windows x86 vtables cannot be used as native Android pointers'])

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--client',type=Path,required=True)
    ap.add_argument('--server',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    result=audit(args.client.read_bytes(),args.server.read_bytes())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('8 original client HUD classes and 2 server cache classes verified')
if __name__=='__main__':main()
