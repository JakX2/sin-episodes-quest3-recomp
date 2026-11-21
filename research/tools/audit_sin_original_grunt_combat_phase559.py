#!/usr/bin/env python3
"""SHA-pinned read-only original 2006 SiN Grunt combat/linked-entity audit.

No original PE32 code, disassembly or game assets are embedded/distributed.
The original ZIP is an owner-side opt-in and must never be added to GitHub.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

# The Phase 558 auditor is in the same repository directory. Reuse its exact
# .text/PE32 reader and both original full-file identity values.
from audit_sin_original_grunt_sentence_phase558 import (PE32, CLIENT_HASH,
    SERVER_HASH, check_edge)

METHODS = {
    0x2235A560: (1327,'99e554b194d86b725559e53c3b1526f7189a974547b745228b475d798e5cb43a',
                 'Grunt prioritized combat decision / fallback-select gate'),
    0x22035250: (46,'4c90dde319ec23bacbf314d4c6a0691139ad2d9dd44d00e8bb139a3386c621b6',
                 'Grunt handle +0x98C serial-checked resolver'),
    0x220A9480: (147,'57e1c9be013f11f99645ffe137ad2d03624606d06202d0af9aed88903014aecc',
                 'linked collection unfiltered or eligible entity count'),
    0x220A9210: (230,'926d34573d524ff0268466c64c61562c97af5df2e62cf482a032e5bc3e44d59b',
                 'first eligible linked entity only if count exceeds one'),
    0x220AA2D0: (78,'3fb8bc6ec71d6b67bd8ca31893ea01520843b09ba405f3906590038da6c9cc1b',
                 'candidate status and linked-group selection predicate'),
}
# Support/reference only; shared CRT-like bytewise comparator *not counted*
# in the five SiN gameplay functions here.
CONTEXT_COMPARATOR=(0x220F5700,59,
  'b567d252928eda7f3baaf24a62f9117096a7f968e19227d36c113165b9e4a94f')

# Literal original E8 rel32 callsites, not a raw disassembly dump.
CALLS = {
    0x2235A56C:0x220397C0, 0x2235A579:0x220397C0,
    0x2235A588:0x22035250, 0x2235A5B0:0x22054D40,
    0x2235A5CB:0x220F5700, 0x2235A5E9:0x220F5700,
    0x2235A607:0x220F5700, 0x2235A625:0x220F5700,
    0x2235A643:0x220397C0, 0x2235A65E:0x220397C0,
    0x2235A6F8:0x221387F0, 0x2235A71C:0x2206B5B0,
    0x2235A760:0x22054D40, 0x2235A785:0x22139F60,
    0x2235A79D:0x22054D40, 0x2235A7B8:0x220397C0,
    0x2235A7F2:0x220092D0, 0x2235A7FB:0x220092D0,
    0x2235A84D:0x220092D0, 0x2235A85A:0x220CDC10,
    0x2235A883:0x223A5BAB, 0x2235A8C1:0x220092D0,
    0x2235A8C9:0x220092D0, 0x2235A8CF:0x22014CC0,
    0x2235A8EF:0x222FF670, 0x2235A904:0x220092D0,
    0x2235A90C:0x220092D0, 0x2235A913:0x220ADAD0,
    0x2235A953:0x22029020, 0x2235A97C:0x22054D40,
    0x2235A9C0:0x220397C0, 0x2235A9ED:0x2206B450,
    0x2235AA0F:0x2206B3B0, 0x2235AA26:0x220092D0,
    0x2235AA2D:0x22027DB0, 0x2235AA34:0x22027D60,
    0x2235AA6B:0x22054D40,
    0x220AA30F:0x220A9210,
}
# These were checked in earlier phases and count as *cross references*, not
# new recovered function extents or new method code.
CROSS_CALLS = {
    0x2235D442:0x2235A560,  # Grunt +0x844 fallback uses nonzero result
    0x220A346C:0x220A9480,  # Phase558 mode-2+ related count
    0x220A34AC:0x220A9480,  # Phase558 mode-3 related count
    0x220A34BC:0x220AA2D0, # Phase558 mode-3 related predicate
}

# Small individual instruction witnesses, separate from entire-method SHA.
# The 0xCC byte at VA 0x2235A81D is part of actual `fmul st,st(4)`,
# NOT an INT3 padding byte. Only the true end 0x2235AA8F is padding.
WITNESSES = {
    0x2235A560: "81ec8c000000",
    0x2235A568: "6a2b",
    0x2235A575: "6a0c",
    0x2235A597: "8b879c000000",
    0x2235A59D: "3de8030000",
    0x2235A5AC: "6a0e",
    0x2235A5C5: "68fced4422",
    0x2235A5E3: "68fc594022",
    0x2235A601: "68f8013f22",
    0x2235A61F: "6828044122",
    0x2235A632: "b870000000",
    0x2235A63F: "6a45",
    0x2235A64D: "b866000000",
    0x2235A65A: "6a16",
    0x2235A667: "8bb6b40c0000",
    0x2235A67A: "25ff0f0000",
    0x2235A686: "c1ee0c",
    0x2235A694: "b872000000",
    0x2235A6A2: "b827000000",
    0x2235A6D5: "83f806",
    0x2235A6E8: "81780c341d4622",
    0x2235A74E: "6a01",
    0x2235A7B4: "6a14",
    0x2235A7D9: "80beb10d000000",
    0x2235A81C: "d8cc",
    0x2235A820: "d81d58af3d22",
    0x2235A83E: "680000fa43",
    0x2235A895: "8b872c060000",
    0x2235A8A3: "f6c401",
    0x2235A932: "ff929c070000",
    0x2235A953: "e8c8e6ccff",
    0x2235A95B: "b874000000",
    0x2235A998: "c686660c000001",
    0x2235A9BC: "6a0a",
    0x2235AA75: "b82a000000",
    0x2235AA85: "33c0",
    0x2235AA8E: "c3",
    0x22035250: "8b818c090000",
    0x22035263: "81e1ff0f0000",
    0x22035273: "394104",
    0x22035278: "8b01",
    0x220A9219: "395d48",
    0x220A9230: "8b07",
    0x220A925E: "83be0402000000",
    0x220A927E: "ff9294060000",
    0x220A9284: "84c0",
    0x220A92E3: "83f801",
    0x220A92E6: "0f9ec1",
    0x220A92EA: "83e901",
    0x220A9485: "807c241000",
    0x220A9492: "397d48",
    0x220A94E6: "8b16",
    0x220A94EA: "ff9294060000",
    0x220AA2D5: "85f6",
    0x220AA2DC: "83be0402000000",
    0x220AA2F1: "ff502c",
    0x220AA2FC: "ff9294060000",
    0x220AA314: "3bc6",
    0x220AA317: "0f94c0",
    0x220F5714: "3a16",
    0x220F5730: "33c0",
}

CLASS_STRINGS = {
    0x2244EDFC:b'func_physbox',
    0x224059FC:b'prop_physics',
    0x223F01F8:b'func_breakable',
    0x22410428:b'func_door',
    0x22461D34:b'npc_turret_floor',
}
FLOATS = {
    0x223DAF58:2500.0,    # 32-bit original squared near-target threshold
}
DOUBLES = {
    0x223D9888:4.0,       # original double in timer gate
    0x223D9EE8:3.0,       # original double gate
    0x22490C58:256.0,    # original double gate
}
RETURN_IDS = {
    0x2235A632:0x70, 0x2235A64D:0x66, 0x2235A694:0x72,
    0x2235A6A2:0x27, 0x2235A76A:0x2A, 0x2235A7A7:0x65,
    0x2235A93F:0x29, 0x2235A95B:0x74, 0x2235A988:0x2A,
    0x2235A99F:0x19, 0x2235AA75:0x2A,
}

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'unexpected original client.dll SHA'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'unexpected original server.dll SHA'
    pe=PE32(server)
    assert pe.base==0x22000000,'unexpected original server image base'
    intervals=[]
    for va,(n,digest,label) in METHODS.items():
        assert pe.section(va)==b'.text',f'{label}: not native .text'
        assert hashlib.sha256(pe.read(va,n)).hexdigest()==digest,f'{label}: body SHA mismatch'
        assert pe.read(va+n,1)==b'\xcc',f'{label}: no authentic next padding'
        intervals.append((va,va+n))
    intervals.sort()
    assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:]))
    # Supporting comparator is authenticated but intentionally excluded from
    # the new SiN gameplay method count and byte total.
    compare_va,compare_size,compare_sha=CONTEXT_COMPARATOR
    assert pe.section(compare_va)==b'.text'
    assert hashlib.sha256(pe.read(compare_va,compare_size)).hexdigest()==compare_sha
    assert pe.read(compare_va+compare_size,1)==b'\xcc'
    for site,target in {**CALLS,**CROSS_CALLS}.items():
        check_edge(pe,site,target,0xe8)
    for site,hexbytes in WITNESSES.items():
        assert pe.read(site,len(bytes.fromhex(hexbytes)))==bytes.fromhex(hexbytes),f'instruction {site:#x}'
    for site,token in CLASS_STRINGS.items():
        assert pe.read(site,len(token)+1)==token+b'\0',f'original classname {site:#x}'
    for site,value in FLOATS.items():
        assert struct.unpack('<f',pe.read(site,4))[0]==value,f'original float {site:#x}'
    for site,value in DOUBLES.items():
        assert struct.unpack('<d',pe.read(site,8))[0]==value,f'original double {site:#x}'
    for site,result in RETURN_IDS.items():
        assert pe.read(site,5)==b'\xb8'+struct.pack('<I',result),f'original return id {site:#x}'
    assert pe.read(0x2235A81C,2)==b'\xd8\xcc', "embedded x87 0xCC is not INT3 padding"
    assert pe.read(0x2235AA8F,1)==b'\xcc', 'actual 1327-byte method end'
    assert sum(n for n,_,_ in METHODS.values())==1828
    assert len(CALLS)==38
    return dict(gameplay_methods=5,complete_distinct_server_bytes=1828,
      verified_internal_e8_calls=len(CALLS),previous_phase_cross_calls=len(CROSS_CALLS),
      instruction_witnesses=len(WITNESSES),original_classname_literals=len(CLASS_STRINGS),
      original_float_and_double_constants=len(FLOATS)+len(DOUBLES),numeric_return_witnesses=len(RETURN_IDS),
      extra_context_comparator_sha_verified=True)

def selftest():
    class Fake:
        def __init__(self,site,blob):self.site,self.blob=site,blob
        def read(self,addr,n):
            assert addr==self.site and n==5
            return self.blob
    for site,target in {**CALLS,**CROSS_CALLS}.items():
        good=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Fake(site,good),site,target,0xe8)
        for i in range(5):
            bad=bytearray(good);bad[i]^=1
            try: check_edge(Fake(site,bytes(bad)),site,target,0xe8)
            except AssertionError: pass
            else: raise AssertionError('mutated rel32 accepted')
    assert sum(x[0] for x in METHODS.values())==1828
    assert len(RETURN_IDS)==11
    try:audit(b'MZ invalid',b'MZ invalid')
    except AssertionError:pass
    else:raise AssertionError('wrong original DLL pair accepted')
    return 'SELF_TEST_OK'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if not (args.bin_zip or args.self_test):p.error('pass --self-test or --bin-zip')
    if args.self_test:print(selftest())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            out=audit(z.read('client.dll'),z.read('server.dll'))
        print('ORIGINAL_SIN_GRUNT_COMBAT_PHASE559_AUTHENTICATED',out)
if __name__=='__main__':main()