#!/usr/bin/env python3
"""Original SiN 2006 Grunt linked-group fixed handle array verification (Phase563).

Requires authorized original retail pair only for opt-in --bin-zip.
Prints checksums and metadata, not commercial original machine-code bytes.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, CLIENT_HASH, SERVER_HASH, edge

METHODS = {
 0x220A8610: (64,'4305455c6f88e006d53dda04c538316e784bf0863d3410fb903e0ea297e58479','serial-checked handle-pointer inequality'),
 0x220A8670: (48,'ee0e6c8a47b6655583a351927a51809764bf3b057e61d47366fa7b96b2bf32bb','inline handle-array remove/compact without owner callback'),
 0x220A9720: (93,'e052f8bb0ad326ec237e6016090f371910004fb3160759bc3952f20f72427190','broadcast member entity-flag OR across valid generation handles'),
 0x220A9790: (68,'157624f5d89aa929c6323f5ca95ae352c09624477dfed4e3ee4f34d4a7ddb90e','inline handle-array append and return original index'),
 0x220A97E0: (129,'2d84f0e2a9602983d23ae2b479c8ee197f27eb4ad8fdf1e8117038b2e13231c4','resolve handle pointers and return first matching member index'),
}
CALLS = {
 0x220A8690: 0x223A5D10,
 0x220A97B6: 0x223A5D10,
}
PRIOR_CALLS = {
 0x220AA0F8: 0x220A97E0, # phase562 leave
 0x220AA209: 0x220A8670, # phase562 full-group eviction
 0x220AA226: 0x220A9790, # phase562 join
}
WITNESSES = {
 0x220A8610:'8b01',
 0x220A8617:'8b1520564f22',
 0x220A861f:'81e1ff0f0000',
 0x220A862c:'c1e80c',
 0x220A863f:'c20400',
 0x220A864d:'c20400',
 0x220A8673:'8b4640',
 0x220A867a:'2bc1',
 0x220A868b:'8d5104',
 0x220A8698:'834640ff',
 0x220A869d:'c20400',
 0x220A9723:'397148',
 0x220A9765:'099888090000',
 0x220A977a:'c20400',
 0x220A9794:'8b7e40',
 0x220A9797:'8d4701',
 0x220A979a:'894640',
 0x220A97a4:'897644',
 0x220A97c9:'8b11',
 0x220A97cd:'8bc7',
 0x220A97d1:'c20400',
 0x220A97e1:'8b5140',
 0x220A9807:'81e2ff0f0000',
 0x220A9816:'c1ee0c',
 0x220A9824:'8b1481',
 0x220A9849:'3bf7',
 0x220A9856:'83c8ff',
 0x220A985e:'c20400',
}
# Two complete methods are adjacent to the following native method with NO INT3.
ADJACENT = {
 0x220A8610: b'\x8b\xc1\xc7\x40\x40',
 0x220A8670: b'\xc7\x41\x40\x00\x00',
}

def audit(client, server):
    assert hashlib.sha256(client).hexdigest() == CLIENT_HASH, 'invalid original client hash'
    assert hashlib.sha256(server).hexdigest() == SERVER_HASH, 'invalid original server hash'
    pe = PE32(server)
    assert pe.base == 0x22000000, 'original server PE image base'
    regions = []
    for va, (size, digest, label) in METHODS.items():
        assert pe.section(va) == b'.text', label
        assert hashlib.sha256(pe.read(va, size)).hexdigest() == digest, label
        assert pe.read(va+size-3, 3) == b'\xc2\x04\x00', f'original ret4 {va:#x}'
        if va in ADJACENT:
            assert pe.read(va+size, len(ADJACENT[va])) == ADJACENT[va], f'next native entry {va:#x}'
        else:
            assert pe.read(va+size, 1) == b'\xcc', f'original padded end {va:#x}'
        regions.append((va,va+size))
    regions.sort()
    assert all(a[1] <= b[0] for a,b in zip(regions, regions[1:])), 'overlap'
    for site,target in {**CALLS, **PRIOR_CALLS}.items():
        edge(pe,site,target)
    for va,hx in WITNESSES.items():
        assert pe.read(va,len(bytes.fromhex(hx))) == bytes.fromhex(hx),f'opcode witness {va:#x}'
    assert sum(n for n,_,_ in METHODS.values()) == 402
    return {'new_complete_server_methods':len(METHODS),
            'new_original_server_bytes':402,
            'direct_calls_in_new_methods':len(CALLS),
            'prior_phase_callsite_witnesses':len(PRIOR_CALLS),
            'opcode_witnesses':len(WITNESSES),
            'adjacent_without_padding':len(ADJACENT),
            'array_capacity_check_in_append':False,
            'callbacks_in_eviction_helper':False}

def self_test():
    class Stub:
        def __init__(self,addr,data):self.addr,self.data=addr,data
        def read(self,addr,n):return self.data[addr-self.addr:addr-self.addr+n]
    for site,target in {**CALLS,**PRIOR_CALLS}.items():
        data=b'\xe8'+struct.pack('<i',target-site-5)
        edge(Stub(site,data),site,target)
        for byte in range(5):
            bad=bytearray(data);bad[byte]^=1
            try:edge(Stub(site,bad),site,target)
            except AssertionError:pass
            else:raise AssertionError('mutant edge accepted')
    try:audit(b'MZjunk',b'MZjunk')
    except AssertionError:pass
    else:raise AssertionError('wrong original DLLs accepted')
    return 'SELF_TEST_OK'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    a=p.parse_args()
    if not(a.self_test or a.bin_zip):p.error('provide --self-test and/or --bin-zip')
    if a.self_test:print(self_test())
    if a.bin_zip:
        with zipfile.ZipFile(a.bin_zip) as z:
            print('SIN_GRUNT_HANDLE_ARRAY_PHASE563_AUTHENTICATED',audit(z.read('client.dll'),z.read('server.dll')))

if __name__ == '__main__':main()