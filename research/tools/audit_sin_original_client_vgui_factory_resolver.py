#!/usr/bin/env python3
"""Read-only authentication of exact 2006 SiN client VGUI factory-array resolver.

Original DLLs are private; verification emits metadata only. No translation,
execution, game launch, or server/client replacement occurs here.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, CLIENT_SHA, SERVER_SHA

VA = 0x240010D0
SIZE = 586
SHA = '73c405920189d4c1c0eeefb994849544d8ff41cf6fc20b79d420e19d52fae64b'
# Exact owner-factory loop order. Factory may be NULL, resolver scans until a non-null result.
# (name ptr, PUSH name site, indirect CALL EAX site, result-global store site, destination, name)
QUERIES = (
    (0x242D3300,0x24001150,0x24001155,0x2400116B,0x2438AE04,'VGUI_ivgui006'),
    (0x242D32F0,0x2400117B,0x24001180,0x24001196,0x2438AE08,'VGUI_Panel009'),
    (0x242D32E0,0x240011A9,0x240011AE,0x240011C4,0x2438ADFC,'VGUI_Surface028'),
    (0x242D32D0,0x240011D9,0x240011DE,0x240011F4,0x2438ADF8,'VGUI_Scheme010'),
    (0x242D32C0,0x24001209,0x2400120E,0x24001224,0x2438AE00,'VGUI_System010'),
    (0x242D32B0,0x24001239,0x2400123E,0x24001254,0x2438ADF4,'VGUI_Input005'),
    (0x242D329C,0x24001269,0x2400126E,0x24001284,0x2438AE10,'VGUI_Localize003'),
    (0x242D328C,0x24001299,0x2400129E,0x240012B8,0x2438AE0C,'VFileSystem012'),
)
# After VGUI_ivgui006::virtual +0x04, the original checks these non-null in
# the exact order recorded. The first pointer was tested immediately before
# the call; it is also redundantly checked below.
REQUIRED_CHECKS = (
    (0x240012C6,0x2438ADF8),
    (0x240012CF,0x2438ADFC),
    (0x240012D8,0x2438AE00),
    (0x240012E1,0x2438ADF4),
    (0x240012EA,0x2438AE04),
    (0x240012F3,0x2438AE0C),
    (0x240012FC,0x2438AE10),
    (0x24001305,0x2438AE08),
)
class Refusal(ValueError): pass

def need(cond,why):
    if not cond: raise Refusal(why)

def at_u32(pe,va): return struct.unpack('<I',pe.at(va,4))[0]

def call_target(pe,site,target):
    b=pe.at(site,5)
    need(b[:1]==b'\xe8' and site+5+struct.unpack_from('<i',b,1)[0]==target,
         f'direct-call witness changed at {site:#x}')

def verify(server:bytes,client:bytes)->dict:
    need(hashlib.sha256(server).hexdigest()==SERVER_SHA,'untrusted original server SHA')
    need(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'untrusted original client SHA')
    pe=PE32(client)
    need(pe.base==0x24000000 and pe.section(VA)=='.text','unexpected client PE32 mapping')
    need(hashlib.sha256(pe.at(VA,SIZE)).hexdigest()==SHA,'original complete resolver SHA mismatch')
    need(pe.at(VA+SIZE,1)==b'\xcc','resolver method boundary changed')
    # Direct caller passes "CLIENT" as first of three arguments.
    need(pe.at(0x242AC524,7)==b'CLIENT\0','CLIENT original literal changed')
    need(pe.at(0x241EDFB7,5)==b'\x68'+struct.pack('<I',0x242AC524),
         'outer VGUI context label changed')
    call_target(pe,0x241EDFBC,VA)
    need(pe.at(0x241EDFC4,4)==b'\x84\xc0\x75\x03',
         'outer VGUI prerequisite result gating changed')
    # The old routine really accepts [esp+8]=factory pointer array,
    # [esp+12]=signed count, then scans each entry in order.
    need(pe.at(0x24001136,8)==b'\x8b\x7c\x24\x44\x8b\x5c\x24\x40',
         'factory array pointer/count acquisition changed')
    need(pe.at(0x24001147,5)==b'\x8b\x04\xb3\x85\xc0',
         'scan-array entry / NULL skip changed')
    for nameva,pushva,callva,storeva,globalva,name in QUERIES:
        need(pe.at(nameva,len(name)+1)==name.encode()+b'\0',f'literal changed: {name}')
        need(pe.at(pushva,5)==b'\x68'+struct.pack('<I',nameva),f'factory name-push changed: {name}')
        need(pe.at(pushva-2,2)==b'\x6a\x00',f'factory return-code parameter changed: {name}')
        need(pe.at(callva,2)==b'\xff\xd0',f'factory indirect CALL changed: {name}')
        need(pe.at(storeva,5)==b'\xa3'+struct.pack('<I',globalva),f'factory result global changed: {name}')
    # Factory #1's pointer is needed for vtable+4 with array and count args.
    need(pe.at(0x240012B0,6)==b'\x8b\x0d'+struct.pack('<I',0x2438AE04),
         'original primary VGUI pointer load changed')
    need(pe.at(0x240012BF,8)==b'\x8b\x11\x57\x53\xff\x52\x04\x83',
         'original VGUI virtual+4 dispatch changed')
    need(pe.at(0x240012BD,2)==b'\x74\x55',
         'original primary VGUI null bailout changed')
    for cmpva,globalva in REQUIRED_CHECKS:
        need(pe.at(cmpva,7)==b'\x83\x3d'+struct.pack('<I',globalva)+b'\x00',
             f'final required global changed at {cmpva:#x}')
        je=pe.at(cmpva+7,2)
        need(je[:1]==b'\x74' and cmpva+9+struct.unpack('<b',je[1:])[0]==0x24001314,
             f'final failure branch changed at {cmpva:#x}')
    need(pe.at(0x2400130E,6)==b'\x5f\x5e\xb0\x01\x5b\xc3','original success return changed')
    need(pe.at(0x24001314,6)==b'\x5f\x5e\x32\xc0\x5b\xc3','original failure return changed')
    return {
      'status':'ORIGINAL_CLIENT_VGUI_FACTORY_RESOLVER_AUTHENTICATED',
      'complete_method_address':hex(VA),'complete_method_bytes':SIZE,
      'complete_method_sha256':SHA,
      'original_client_sha256':CLIENT_SHA,
      'factory_array_scans':len(QUERIES),
      'factory_interface_names':[x[5] for x in QUERIES],
      'required_global_null_checks':len(REQUIRED_CHECKS),
      'primary_vgui_virtual_byte_offset':'0x04',
      'limit':'Metadata and C-like decompilation only; NOT client.dll binary replacement or game boot.'
    }

def self_test():
    class Dummy:
        def __init__(self,buf): self.buf=buf
        def at(self,va,n):return self.buf[:n]
    origin=0x24002000; target=0x24003100
    good=b'\xe8'+struct.pack('<i',target-origin-5)
    call_target(Dummy(good),origin,target)
    for i in range(len(good)):
        bad=bytearray(good); bad[i]^=1
        try:call_target(Dummy(bytes(bad)),origin,target)
        except Refusal:pass
        else:raise AssertionError('mutated call accepted at byte '+str(i))
    try:verify(b'not-original',b'not-original')
    except Refusal:pass
    else:raise AssertionError('unexpectedly accepted wrong DLL hashes')
    assert len(QUERIES)==8 and len(REQUIRED_CHECKS)==8
    assert len({x[4] for x in QUERIES})==8
    print('PASS 8 VGUI interface witnesses, 8 fail-closed globals, mutated-call rejection and SHA refusal')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-zip',type=Path)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if not args.bin_zip and not args.self_test:parser.error('use --self-test or --bin-zip')
    if args.self_test:self_test()
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            p={Path(name).name.lower():name for name in z.namelist()}
            need('server.dll' in p and 'client.dll' in p,'original pair missing')
            report=verify(z.read(p['server.dll']),z.read(p['client.dll']))
        print(json.dumps(report,indent=2))

if __name__=='__main__':main()