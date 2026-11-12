#!/usr/bin/env python3
"""Phase188: audit the paired original server scene/model-sounds cache loaders.

The supplied 2006 PE32 DLLs must remain private. No complete original code
bodies are committed. Exact whole-region SHA256, sparse native opcode
witnesses, call edges, static object metadata and normalized twin hashes
are used to pin a host-neutral behavioral reconstruction.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_post_emitter import PE32, VerificationError, audit as phase187_audit

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
# Four complete original function bodies, aligned to the following code entrypoint.
REGIONS={
 'scene_init':(0x221AD3E0,576,'7b9db352473a8fc2d5343473b673ac52c89a8c98320847b0b4dd451dcf8e4e69'),
 'modelsounds_init':(0x221498C0,576,'94b55dbf7e21d7b45362d277505a8a7c02537667c3c4c4afbeeded9c40ec2218'),
 'scene_reload':(0x221AD620,80,'94ca224968818a97ef138555040b7c14075dd21dc1f31b273675e2ba32974cf9'),
 'modelsounds_reload':(0x22149B00,80,'1f2ea46fd39f2c9e8708e1e58a8a72ac5c9aed0c447f43cef04b7e6750e328c1'),
}
INIT_ADDRESSES=(0x221AD3E0,0x221498C0)
RELOAD_ADDRESSES=(0x221AD620,0x22149B00)
# Sparse opcode witnesses shared by both methods in each pair, written as
# local offsets from their entrypoints rather than exposing entire bodies.
INIT_WITNESSES={
  0x000:'81ec30020000',
  0x006:'56',
  0x009:'807e3d00',
  0x00F:'b001',
  0x019:'8b4634',
  0x021:'c6463d01',
  0x02B:'803800',
  0x034:'8b4640',
  0x03B:'ffd0',
  0x041:'8b5634',
  0x045:'68200c3f22',
  0x04A:'894644',
  0x05B:'ff5008',
  0x068:'8b0da4215722',
  0x07C:'ff10',
  0x083:'c644240f00',
  0x0A2:'ff10',
  0x0CA:'3944241c',
  0x0D0:'3bc5',
  0x0E8:'8b0da4215722',
  0x0FD:'ff12',
  0x111:'0f8ea5000000',
  0x12D:'896c241c',
  0x157:'ff12',
  0x16E:'8d4c2430',
  0x19C:'894504',
  0x1D9:'8b5634',
  0x1EB:'c644240f01',
  0x1F6:'8b01',
  0x201:'7418',
  0x214:'ff5244',
  0x217:'c6463c01',
  0x21D:'b001',
  0x226:'c3',
  0x227:'6810ba4022',
  0x235:'5d',
  0x23F:'c3',
}
RELOAD_WITNESSES={
  0x000:'56',
  0x001:'8bf1',
  0x003:'807e3d00',
  0x009:'807e3c00',
  0x00D:'c6463d00',
  0x011:'7415',
  0x020:'8bce',
  0x028:'807e4e00',
  0x02E:'8bce',
  0x035:'84c0',
  0x037:'7507',
  0x048:'8bce',
}

# Exact direct relative CALL targets. The two loaders share the same 576-byte
# opcode layout, but use distinct record-identity callbacks.
INIT_CALL_TARGETS={
 0x221AD3E0:{0x106:0x2228DBC0,0x13F:0x2228D7D0,0x15F:0x2228D6A0,
             0x172:0x2228D810,0x17E:0x221ACFD0,0x194:0x2213ACB0,
             0x1C0:0x2207EC10},
 0x221498C0:{0x106:0x2228DBC0,0x13F:0x2228D7D0,0x15F:0x2228D6A0,
             0x172:0x2228D810,0x17E:0x22149770,0x194:0x2213ACB0,
             0x1C0:0x2207EC10},
}
RELOAD_CALL_TARGETS={
 0x221AD620:{0x13:0x221AA5E0,0x1B:0x221A80C0,0x30:0x221A3550,
             0x3B:0x221A8600,0x43:0x221A80C0},
 0x22149B00:{0x13:0x22146410,0x1B:0x22143F50,0x30:0x2213A7B0,
             0x3B:0x22144000,0x43:0x22143F50},
}
RELOAD_TAIL_JMP_OFFSETS=(0x23,0x4B)
NORMALIZED_SHA={
 'init':'935ba6392cdec614fcc9ef5ca1745a48977d7f0b9e16356901188c8e5fe9f484',
 'reload':'3be911fa12f24228389cb3711dc8aba024c133b21b79716088074128aebc9ac3',
}
# Original PE-initialized singleton data and native C-strings.
OBJECTS={
 0x22503EE0:(0x2242ED68,3,'scene.cache'),
 0x224F0388:(0x2240BC9C,2,'modelsounds.cache'),
}
STRINGS={
 0x223F0C20:b'MOD\0',
 0x223F0B4C:b'rb\0',
 0x2240BA10:b"CUtlCachedFileData:  Can't Init, no repository file specified.\0",
 0x2240BA50:b"Discarding repository '%s' due to cache system version change\n\0",
 0x2240BA90:b"Discarding repository '%s' due to version change\n\0",
 0x2240BAC4:b"Discarding repository '%s' due to meta checksum change\n\0",
 0x2242ED68:b'scene.cache\0',
 0x2240BC9C:b'modelsounds.cache\0',
}

def _rel32(opcode:int,source:int,target:int)->bytes:
    return bytes((opcode,))+struct.pack('<i',target-source-5)

def verify_native(pe:PE32,*,whole_hash:bool=True)->dict:
    if pe.base!=0x22000000:raise VerificationError('Phase188 server image base mismatch')
    for name,(va,n,digest) in REGIONS.items():
        if whole_hash and hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
            raise VerificationError('Phase188 whole native function SHA mismatch: '+name)
    for base in INIT_ADDRESSES:
        for off,hexcode in INIT_WITNESSES.items():
            if pe.at(base+off,len(bytes.fromhex(hexcode)))!=bytes.fromhex(hexcode):
                raise VerificationError('Phase188 native Init opcode mismatch at %#x'%(base+off))
        for off,target in INIT_CALL_TARGETS[base].items():
            if pe.at(base+off,5)!=_rel32(0xe8,base+off,target):
                raise VerificationError('Phase188 Init CALL mismatch at %#x'%(base+off))
    for base in RELOAD_ADDRESSES:
        for off,hexcode in RELOAD_WITNESSES.items():
            if pe.at(base+off,len(bytes.fromhex(hexcode)))!=bytes.fromhex(hexcode):
                raise VerificationError('Phase188 native Reload opcode mismatch at %#x'%(base+off))
        for off,target in RELOAD_CALL_TARGETS[base].items():
            if pe.at(base+off,5)!=_rel32(0xe8,base+off,target):
                raise VerificationError('Phase188 Reload CALL mismatch at %#x'%(base+off))
        for off in RELOAD_TAIL_JMP_OFFSETS:
            if pe.at(base+off,5)!=_rel32(0xe9,base+off,INIT_ADDRESSES[RELOAD_ADDRESSES.index(base)]):
                raise VerificationError('Phase188 Reload tail JMP mismatch at %#x'%(base+off))
    for object_va,(path_va,version,path) in OBJECTS.items():
        if pe.at(object_va+0x34,4)!=struct.pack('<I',path_va):
            raise VerificationError('Phase188 singleton path pointer mismatch')
        if pe.at(object_va+0x38,4)!=struct.pack('<I',version):
            raise VerificationError('Phase188 singleton expected repository version mismatch')
        if pe.at(path_va,len(path)+1)!=path.encode('ascii')+b'\0':
            raise VerificationError('Phase188 singleton pathname bytes mismatch')
    for va,literal in STRINGS.items():
        if pe.section(va)!='.rdata' or pe.at(va,len(literal))!=literal:
            raise VerificationError('Phase188 original repository diagnostic/string mismatch at %#x'%va)
    if whole_hash:
        # Compare both full functions after erasing only the original four-byte
        # relative CALL displacements, never instruction bytes or other data.
        for group,bases,n,callsets in (
          ('init',INIT_ADDRESSES,576,INIT_CALL_TARGETS),
          ('reload',RELOAD_ADDRESSES,80,RELOAD_CALL_TARGETS)):
            normalized=[]
            for base in bases:
                data=bytearray(pe.at(base,n))
                for off in callsets[base]:
                    if data[off]!=0xe8:raise VerificationError('Phase188 malformed normalized call')
                    data[off+1:off+5]=bytes(4)
                normalized.append(bytes(data))
            if normalized[0]!=normalized[1] or hashlib.sha256(normalized[0]).hexdigest()!=NORMALIZED_SHA[group]:
                raise VerificationError('Phase188 normalized twin cache function mismatch '+group)
    return {
      'complete_native_functions':4 if whole_hash else 0,
      'complete_original_x86_bytes':1312 if whole_hash else 0,
      'sparse_opcode_witnesses':2*(len(INIT_WITNESSES)+len(RELOAD_WITNESSES)),
      'direct_relative_calls':sum(map(len,INIT_CALL_TARGETS.values()))+sum(map(len,RELOAD_CALL_TARGETS.values())),
      'reload_tail_jumps':4,
      'normalized_identical_576_byte_initializers':bool(whole_hash),
      'normalized_identical_80_byte_reloaders':bool(whole_hash),
      'repositories':{s[2]:{'original_object_va':hex(va),'repository_version':s[1]} for va,s in OBJECTS.items()},
      'limitations':['Opaque source file-system vtable and per-record loader remain unported',
                    'Native missing-path false return sets initialized byte before returning',
                    'Model uses host-only record bound; original record decoder is not claimed',
                    'No engine-hosted original Windows game DLL execution'],
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA or hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA:
        raise VerificationError('Phase188 original full server/client DLL SHA256 mismatch')
    # Retain all Phase187 known original startup gate checks.
    report=phase187_audit(server_bytes,client_bytes)
    report['original_server_cache_repositories']=verify_native(PE32(server_bytes))
    return report

def main()->None:
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--server',type=Path,required=True)
    a.add_argument('--client',type=Path,required=True)
    a.add_argument('--output',type=Path)
    args=a.parse_args()
    report=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+'\n'
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(report,encoding='utf8')
    else:print(report,end='')
if __name__=='__main__':main()