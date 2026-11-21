#!/usr/bin/env python3
"""Original 2006 SiN server CVoiceGameMgr ClientCommand + state-bit audit.

Only original *metadata* (method SHA-256, addresses, selected instruction
witnesses) lives here. Requires owner-local original server/client bin.zip.
Never upload either original DLL or their raw machine code to a repository.
"""
import argparse
import hashlib
import struct
import sys
import zipfile
from pathlib import Path

# Original PE reader and call checker were established by Phase551.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_sin_original_voice_block_hash import (  # noqa: E402
    CLIENT_HASH, SERVER_HASH, PE32, check_edge,
)

# Exact contiguous original method bytes, ending immediately before native CC
# alignment. Here the new complete methods have a CC after the actual ret.
METHODS = {
    0x222938C0: (25, '244461d7dac17754a8145cb8b1d5231bab16312fe290d9fbdfa18cf2bb77a874', 'CVoiceGameMgr small constructor'),
    0x222939B0: (44, 'f9e26f4bf9cb49b43babb3767b91430846ed0ef3fc657d09b4b45b6a18d64eec', 'bit-field write helper'),
    0x222939E0: (28, '330963a2a959d329b0a0a22efa34b4d41697a06a1c31780be324183d05dea26c', 'bit-field read helper'),
    0x22293A00: (78, '6bfdc3b53311c56222ec6c1eead63d41feca77481e4a28f92e2c623ade072871', 'server voice debug log formatter'),
    0x22293A50: (31, '0fb959725ca93ab3dc5e70a6d74c7279b08c5d8e3cfa946b434bda45e967d09a', 'CVoiceGameMgr deleting destructor'),
    0x22293AC0: (66, '8138935fb4f1b22d370719b96f38020a013937180b2addb2eb6873f2c6de61d0', 'request VoiceMask/RequestState resynchronization'),
    0x223A5574: (67, 'd108c0f0291d63e422757a9255fa568f383a772601ba2a66a6f2b79ca9fe7f29', 'native locale-aware casefold command compare dispatcher'),
    0x223A8410: (78, '9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658', 'native ASCII casefold command compare fallback'),
    0x22293B10: (473, '3f06ebffe8e1a23a00aeead64bd1992ebb792ced6099fc26112ea2108a37f950', 'CVoiceGameMgr ClientCommand'),
}

DIRECT_CALLS = {
    0x22293A05: 0x223A6760,  # stack-size preparation for diagnostic
    0x22293A2F: 0x223A75AF,  # format diagnostics
    0x22293A61: 0x2226E800,  # deleting destructor free
    0x22293B4B: 0x223A5574,  # vban name comparison
    0x22293BB7: 0x223A5CD6,  # hex-argument scanf-style conversion
    0x22293BCF: 0x22293A00,  # first-mask verbose diagnostic
    0x22293BE5: 0x22293A00,  # additional-argument diagnostic
    0x22293C16: 0x223A5574,  # VModEnable name comparison
    0x22293C4E: 0x223A56CB,  # numeric string conversion
    0x22293C5F: 0x22293A00,  # VModEnable verbose diagnostic
    0x22293C84: 0x223A56CB,  # numeric argument conversion for bit update
    0x22293C97: 0x222939B0,  # original per-recipient bit setter
    0x22293CD7: 0x22293A00,  # invalid sender diagnostic
    0x223A5591: 0x223A72DA,  # locale character-fold first operand
    0x223A559D: 0x223A72DA,  # locale character-fold second operand
}

DIRECT_JUMPS = {0x223A557D:0x223A8410}  # locale unready -> ASCII fallback

# Site-specific complete x86 instructions or contiguous distinctive groups.
# Original whole-function SHA checks, above, authenticate EVERY instruction;
# these witnesses make important *claims* reproducible without shipping code.
WITNESSES = {
    0x222938C0: 'dd05988f3d22',       # native 0.0 double init
    0x222938CB: 'c700d06a4522',       # native vtable
    0x222939B2: '8b4804',             # index from pair+4
    0x222939B9: 'c1fa05',             # signed 32-bit word index
    0x222939BF: '83e11f',             # bit within word
    0x222939D0: '0910',               # OR mask set
    0x222939D5: 'f7d2',               # NOT mask for clearing
    0x222939D7: '2110',               # AND mask clear
    0x222939E3: '8b5604',             # get bit index
    0x222939F4: 'c1fa05',             # bit-index word shift
    0x222939F8: '230491',             # AND selected word
    0x22293A0F: '83782c00',         # debug global's +0x2C gate
    0x22293A29: '6800100000',       # 4096-byte formatting bound
    0x22293A58: 'c706d06a4522',     # deleting destructor sets original vtable
    0x22293AC0: '8b0d98215722',     # engine interface pointer
    0x22293ACE: 'ff5048',           # engine virtual +0x48 sender lookup
    0x22293AD1: '83e801',           # original 1-based to 0-based player
    0x22293AD6: 'c1f905',           # pending word index
    0x22293AE2: '83e11f',           # pending bit index
    0x22293AEE: '0932',             # set pending bit
    0x22293AF0: '890c8588045922',   # cached audible[recipient] = 0
    0x22293AF7: '890c8508055922',   # cached banned[recipient] = 0
    0x22293AFF: 'c20400',           # single argument ret
    0x22293B14: '8b806c010000',     # client +0x16C network handle
    0x22293B32: '0f8894010000',     # negative-index guard
    0x22293B38: '3b7e0c',           # client count upper guard
    0x22293B45: '68bc6b4522',       # vban command string
    0x22293B63: 'ff90c4000000',     # argc via virtual +0xC4
    0x22293BA0: '68b86b4522',       # scanf-style %x literal
    0x22293BC2: '7f1b',             # only arg #1 stores a ban mask
    0x22293BDB: '890b',             # save parsed ban mask for sender
    0x22293C10: '68446b4522',       # VModEnable command string
    0x22293C2E: 'ff90c4000000',     # argc via +0xC4
    0x22293C47: 'ff92c8000000',     # argv(1) via virtual +0xC8
    0x22293C71: 'c744240c00045922', # bit-pair base -> 0x22590400
    0x22293C97: 'e814fdffff',       # write bit via native helper
    0x22293C9E: 'c1f905',           # pending bitset word
    0x22293CAA: '83e11f',           # pending bit offset
    0x22293CB8: '2110',             # clear pending bit
    0x22293CC3: '32c0',             # unknown/under-argc -> false
    0x22293CD2: '68d46a4522',       # invalid sender diagnostic
    0x22293CE0: 'b001',             # invalid sender -> true (!) 
    0x22293CE6: 'c20800',           # two arguments ret
    0x223A5574: '833d00f5592200',  # native locale readiness test
    0x223A557D: 'e98e2e0000',      # branch to ASCII fallback
    0x223A55A9: '7404',            # loop until mismatch / zero
    0x223A8410: '558bec',          # native ASCII fallback prologue
    0x223A8432: '2c41',            # native ASCII lower-case threshold
    0x223A8441: '2c41',            # second operand ASCII folding
    0x223A845D: 'c3',              # native fallback return
}

STRINGS = {
    0x22456AD4: b'CVoiceGameMgr::ClientCommand: cmd %s from invalid client (%d)\n',
    0x22456B14: b'CVoiceGameMgr::ClientCommand: VModEnable (%d)\n',
    0x22456B44: b'VModEnable',
    0x22456B50: b'CVoiceGameMgr::ClientCommand: invalid index (%d)\n',
    0x22456B84: b'CVoiceGameMgr::ClientCommand: vban (0x%x) from %d\n',
    0x22456BB8: b'%x',
    0x22456BBC: b'vban',
}

# Re-check the preceding complete function already included in Phase555;
# do not count its bytes as newly recovered here.
PRIOR_VOICE_MASK_SENDER = (
    0x22293CF0, 619,
    '4b9346aca0bdbdf927aa5932929c452c1b10a1021d9973a2bbe8ce25fcceb8c8',
)

# MSVC native RTTI chain: vtable at 0x22456AD0, -4 COL at 0x22456ACC.
VTABLE_VA=0x22456AD0
COL_VA=0x224C9A6C
TYPE_DESCRIPTOR=0x2251CC9C


def audit(client:bytes, server:bytes)->dict:
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH, 'unsupported original client SHA256'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH, 'unsupported original server SHA256'
    s=PE32(server);c=PE32(client)
    assert s.base==0x22000000 and c.base==0x24000000
    intervals=[]
    adjacent={0x223A5574:b'\x56',0x223A8410:b'\x68'}
    for va,(size,digest,label) in METHODS.items():
        assert s.section(va)==b'.text', f'{label} outside .text'
        assert hashlib.sha256(s.read(va,size)).hexdigest()==digest, f'{label} exact original code mismatch'
        if va in adjacent:
            assert s.read(va+size-1,1)==b'\xC3', f'{label} missing RET'
            assert s.read(va+size,1)==adjacent[va], f'{label} unexpected adjacent method entry'
        else:
            assert s.read(va+size,1)==b'\xCC', f'{label} has unexpected method boundary'
        intervals.append((va,va+size))
    intervals.sort()
    assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:])), 'overlapping method accounting'
    for va,target in DIRECT_CALLS.items():check_edge(s,va,target,0xE8)
    for va,target in DIRECT_JUMPS.items():check_edge(s,va,target,0xE9)
    for va,hx in WITNESSES.items():
        expected=bytes.fromhex(hx)
        assert s.read(va,len(expected))==expected, f'wrong original instruction witness {va:#x}'
    for va,name in STRINGS.items():
        assert s.read(va,len(name)+1)==name+b'\0', f'missing native diagnostic/command string {va:#x}'
    assert struct.unpack('<I',s.read(VTABLE_VA-4,4))[0]==COL_VA, 'incorrect MSVC vtable COL'
    assert struct.unpack('<I',s.read(VTABLE_VA,4))[0]==0x22293A50, 'incorrect original deleting destructor'
    assert struct.unpack('<I',s.read(COL_VA+12,4))[0]==TYPE_DESCRIPTOR, 'incorrect RTTI descriptor pointer'
    assert s.read(TYPE_DESCRIPTOR+8,len(b'.?AVCVoiceGameMgr@@')+1)==b'.?AVCVoiceGameMgr@@\0', 'wrong original class name'
    assert struct.unpack('<I',s.read(0x222938CD,4))[0]==VTABLE_VA, 'original constructor writes wrong vtable'
    va,n,sha=PRIOR_VOICE_MASK_SENDER
    assert hashlib.sha256(s.read(va,n)).hexdigest()==sha, 'Phase555 sender does not match original'
    assert sum(size for size,_,_ in METHODS.values())==890
    return {'complete_new_methods':len(METHODS),'unique_original_code_bytes':890,
            'native_call_edges':len(DIRECT_CALLS),'native_jump_edges':len(DIRECT_JUMPS),'opcode_witnesses':len(WITNESSES),
            'native_diagnostic_strings':len(STRINGS),'rtti_class':'CVoiceGameMgr',
            'prior_VoiceMask_sender_rechecked':True}


def selftest():
    try:audit(b'MZ bad client',b'MZ bad server')
    except AssertionError:pass
    else:raise AssertionError('unsupported DLLs accepted')
    class Synthetic:
        def __init__(self,site,data):self.site,self.data=site,data
        def read(self,a,n):return self.data[a-self.site:a-self.site+n]
    for opcode,edges in ((0xE8,DIRECT_CALLS),(0xE9,DIRECT_JUMPS)):
      for va,target in edges.items():
        raw=bytes([opcode])+struct.pack('<i',target-va-5)
        check_edge(Synthetic(va,raw),va,target,opcode)
        for pos in range(5):
            bad=bytearray(raw);bad[pos]^=1
            try:check_edge(Synthetic(va,bad),va,target,opcode)
            except AssertionError:pass
            else:raise AssertionError('bad rel32 mutation accepted')
    assert len(METHODS)==9 and sum(x[0] for x in METHODS.values())==890
    return 'SELF_TEST_OK'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if not(args.bin_zip or args.self_test):p.error('use --bin-zip and/or --self-test')
    if args.self_test:print(selftest())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            result=audit(z.read('client.dll'),z.read('server.dll'))
        print('ORIGINAL_SERVER_CVOICEGAMEMGR_COMMANDS_AUTHENTICATED',result)

if __name__=='__main__':main()