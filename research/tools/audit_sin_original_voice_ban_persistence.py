#!/usr/bin/env python3
"""SHA-pinned read-only authentication of original SiN 2006 voice_ban.dt I/O.

Reuses the original PE32/edge validator committed with Phase551.  Only
private, owner-provided binaries may be used with --bin-zip.  No original DLL
bytes are embedded in this tool or in its synthetic tests.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
import sys
import zipfile

# Standalone local package can import the copied Phase551 verifier too.
try:
    from audit_sin_original_voice_block_hash import PE32, CLIENT_HASH, SERVER_HASH, check_edge
except ImportError as exc:
    raise SystemExit('Phase551 audit_sin_original_voice_block_hash.py must be available on PYTHONPATH') from exc

# Separate, nonoverlapping original .text ranges and complete-method SHA256s.
METHODS = {
    0x241E4D00: (130, 'a2e7e39084e17a42aea5aecc7dd5ff5a94dc6e0be414f7d8caad2f842a619629', 'voice-ban v1 export'),
    0x241E4F80: (5,   '0873af1117bd7228ee29e0b904b306310d0b12e225a6dd1bb9836b1a2e9ddb6c', 'hash reset forwarding thunk'),
    0x241E4F90: (187, '0cdc0f2ce180fb3eb921a0311b65735fa8b46d275b4220cac5f7d32909f0d471', 'voice-ban v1 import'),
}
# Calls in the new methods and two verified caller links inside existing
# independently SHA-verified Phase552 methods (not recounted here).
CALLS = {
    0x241E4F97: 0x241E4EC0,     # import clears previously blocked identities FIRST
    0x241E502A: 0x241E4E30,     # import inserts next 32-byte key
    0x241E50B1: 0x241E4D00,     # CVoiceStatus destructor calls writer
    0x241E5C79: 0x241E4F90,     # CVoiceStatus Init calls reader
}
JUMPS = {0x241E4F80: 0x241E4EC0}
# All of these addresses were read from the private original 2006 DLL.
WITNESSES = {
    0x241E4D01: '8b1538d23124',     # filename pointer global
    0x241E4D09: '6a00',             # zero native fopen option
    0x241E4D0D: '8b0dbcf63324',     # engine-acquired file-provider interface
    0x241E4D15: '68ac032a24',       # literal 'wb'
    0x241E4D1B: 'ff5008',           # provider virtual +0x08 open
    0x241E4D20: '85db',             # null handle check
    0x241E4D33: 'c744241801000000', # native little-endian version 1
    0x241E4D3E: 'ff5004',           # file write operation
    0x241E4D41: 'bd00010000',       # 256 buckets
    0x241E4D46: '8b7724',           # first bucket element
    0x241E4D58: '53',               # file handle
    0x241E4D59: '6a20',             # exactly 32 key bytes
    0x241E4D5C: 'ff5004',           # write record
    0x241E4D66: '83c728',           # 40-byte next bucket
    0x241E4D77: 'ff520c',           # close
    0x241E4D7F: 'c20400',           # ret one argument
    0x241E4F80: 'e93bffffff',       # tiny reset thunk
    0x241E4F90: '83ec24',           # stack buffer 36 bytes
    0x241E4F97: 'e824ffffff',       # clear hash before open
    0x241E4F9C: '8b0dbcf63324',     # same file-provider global
    0x241E4FAA: '6a00',             # option zero
    0x241E4FAC: '680c9c2824',       # literal 'rb'
    0x241E4FB2: 'ff5008',           # open
    0x241E4FB7: '85f6',             # handle null check
    0x241E4FC8: '6a04',             # read 4-byte version
    0x241E4FCF: 'ff10',             # provider virtual +0x00 read
    0x241E4FD1: '837c240801',       # exact version 1
    0x241E4FE1: '6a02',             # native seek-mode enum 2
    0x241E4FE3: '6a00',             # native seek displacement 0
    0x241E4FE6: 'ff5010',           # original provider virtual +0x10
    0x241E4FF2: 'ff5214',           # original provider virtual +0x14
    0x241E5003: '83ef04',           # length minus v1 header
    0x241E5007: 'c1ef05',           # unsigned divide by 32
    0x241E500A: 'ff5010',           # second seek operation
    0x241E501A: '6a20',             # record read of 32 bytes
    0x241E5021: 'ff12',             # provider virtual +0x00 read
    0x241E502A: 'e801feffff',       # insertion helper
    0x241E503E: 'ff520c',           # close
    0x241E5042: 'b001',             # native returns true on completion/failure-to-open
    0x241E5048: 'c20400',           # ret one argument
}
LITERALS = {
    0x242A9EE4: b'voice_ban.dt',
    0x242A03AC: b'wb',
    0x24289C0C: b'rb',
}

def audit(client: bytes, server: bytes) -> dict:
    assert hashlib.sha256(client).hexdigest() == CLIENT_HASH, 'wrong client.dll SHA256'
    assert hashlib.sha256(server).hexdigest() == SERVER_HASH, 'wrong server.dll SHA256'
    pe=PE32(client)
    assert pe.base == 0x24000000
    ranges=[]
    for va,(size,digest,label) in METHODS.items():
        assert pe.section(va) == b'.text', f'{label}: not in .text'
        raw=pe.read(va,size)
        assert hashlib.sha256(raw).hexdigest() == digest, f'{label}: full SHA mismatch'
        assert pe.read(va+size,1) == b'\xcc', f'{label}: unexpected method end'
        ranges.append((va,va+size))
    ranges.sort()
    assert all(a[1] <= b[0] for a,b in zip(ranges,ranges[1:])), 'overlapping methods'
    for va,target in CALLS.items(): check_edge(pe,va,target,0xE8)
    for va,target in JUMPS.items(): check_edge(pe,va,target,0xE9)
    for va,hex_bytes in WITNESSES.items():
        expected=bytes.fromhex(hex_bytes)
        assert pe.read(va,len(expected)) == expected, f'instruction mismatch at {va:#x}'
    for va,s in LITERALS.items():
        assert pe.read(va,len(s)+1) == s+b'\x00', f'string mismatch {va:#x}'
    assert struct.unpack('<I',pe.read(0x2431D238,4))[0] == 0x242A9EE4, 'voice_ban.dt original pointer'
    assert sum(s for s,_,_ in METHODS.values()) == 322
    return {'complete_original_methods':len(METHODS),
            'new_original_code_bytes':322,'direct_call_edges':len(CALLS),
            'direct_tail_jumps':len(JUMPS),'instruction_witnesses':len(WITNESSES),
            'native_string_literals':len(LITERALS),'disk_format_version':1,
            'opaque_key_bytes':32,'filename':'voice_ban.dt'}

def selftest() -> str:
    assert sum(s for s,_,_ in METHODS.values()) == 322
    try: audit(b'MZ fake',b'MZ fake')
    except AssertionError: pass
    else: raise AssertionError('synthetic SHA accepted')
    # Independently check all five bytes of a CALL and a JMP.
    class MockPE:
        def __init__(self,va,data): self.va,self.data=va,data
        def read(self,addr,length): return self.data[addr-self.va:addr-self.va+length]
    for va,target,op in [(0x241E4F97,0x241E4EC0,0xE8),(0x241E4F80,0x241E4EC0,0xE9)]:
        raw=bytes([op])+struct.pack('<i',target-(va+5))
        check_edge(MockPE(va,raw),va,target,op)
        for j in range(5):
            mutated=bytearray(raw); mutated[j]^=1
            try: check_edge(MockPE(va,bytes(mutated)),va,target,op)
            except AssertionError: pass
            else: raise AssertionError(f'unnoticed edge byte {j}')
    return 'SELF_TEST_OK'

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--bin-zip',type=Path)
    a.add_argument('--self-test',action='store_true')
    args=a.parse_args()
    if not(args.self_test or args.bin_zip):a.error('use --self-test and/or --bin-zip')
    if args.self_test:print(selftest())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            result=audit(z.read('client.dll'),z.read('server.dll'))
        print('ORIGINAL_VOICE_BAN_PERSISTENCE_AUTHENTICATED',result)
if __name__=='__main__':main()