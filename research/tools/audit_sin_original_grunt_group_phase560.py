#!/usr/bin/env python3
"""Phase 560: authenticate original SiN 2006 Grunt linked-group methods.

Read-only. The commercial PE32 DLL pair remains private; only exact SHA256
method digests, call targets and short instruction witnesses are recorded.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CLIENT_HASH = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_HASH = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

class PE32:
    def __init__(self, data):
        if data[:2] != b'MZ':
            raise ValueError('invalid DOS header')
        peoff = struct.unpack_from('<I', data, 0x3c)[0]
        if data[peoff:peoff+4] != b'PE\0\0':
            raise ValueError('invalid PE signature')
        assert struct.unpack_from('<H', data, peoff+4)[0] == 0x14c
        opt = peoff + 24
        assert struct.unpack_from('<H', data, opt)[0] == 0x10b
        self.base = struct.unpack_from('<I', data, opt+28)[0]
        count = struct.unpack_from('<H', data, peoff+6)[0]
        pos = opt + struct.unpack_from('<H', data, peoff+20)[0]
        self.sections=[]
        for j in range(count):
            b = pos + 40*j
            name=data[b:b+8].split(b'\x00')[0]
            size, rva, raw_size, raw_off=struct.unpack_from('<IIII',data,b+8)
            self.sections.append((name,self.base+rva,size,raw_size,raw_off))
        self.data=data
    def read(self,va,n):
        for name,base,_,raw,off in self.sections:
            if base <= va and va+n <= base+raw:
                return self.data[off+va-base:off+va-base+n]
        raise ValueError(f'unmapped VA {va:#x}')
    def section(self,va):
        for name,base,_,raw,off in self.sections:
            if base<=va<base+raw:return name
        return None

METHODS={
0x22054CE0:(47,'ceffd4b720ba13adab1a5823d0be9085b058823227fb28b08de84d58dab1710e','global linked-group lookup/fallback attach'),
0x22054D10:(33,'db4185026c3fe22e4efb3bca4b8acc40858ce3cccf0161f874b1a8042ba2baea','Grunt linked-group teardown/clear'),
0x22054D40:(64,'cecfc5c20393a5acd51002f6d48d3dc4e055b1203bfa2efd7b44b4702287e8e5','linked-group restriction gate for a numeric condition'),
0x22054D80:(36,'0ed1c827632ec634707dffe209a14580b7273305e34b124fbfb4910c9d007e12','optional identity pointer equality guard'),
0x22054DB0:(159,'b8ef159948975464611f16dce765e38bc6740b60e66e274ba9e9aaa9225760a8','iterate eligible linked members, early success dispatch'),
0x22054E50:(167,'ecbcae173102f611af54359d90a772aaec9d0ada33544aab50c16ef978487b43','count members matching source scalar, owner-only fallback'),
0x220AAEB0:(240,'04b8c968551b4093eadfea1bb9768b0391e250547f8f13a1cd31e11eabb47ca0','range admission and bit reservation'),
0x220A9300:(188,'f01ab41f5a8a83dd597f93dbe89a0ce82317eeaaf12bfd8e746e49e982aae1ac','first eligible linked entity'),
0x220A93C0:(190,'d9fd368ad8953b75accd68a184dac514a86243ea5b7ffea529f3e33d4d737a6a','next eligible linked entity'),
}

# Genuine E8 rel32 edges wholly in these methods. External edge targets are
# prerequisites but not represented as additional newly recovered methods.
CALLS={
0x22054CEE:0x220A8050,0x22054CFD:0x220AA810,0x22054D05:0x220AA1B0,
0x22054D20:0x220AA0D0,0x22054D68:0x220AAEB0,
0x22054DCF:0x220A9300,0x22054DFF:0x2213FA60,
0x22054E1A:0x22033F10,0x22054E30:0x220A93C0,
0x22054E5E:0x221F3E20,0x22054E72:0x222AA640,0x22054E7D:0x222AA640,
0x22054EAA:0x220A9300,0x22054EB7:0x222AA640,0x22054EC2:0x222AA640,
0x22054EE3:0x220A93C0,
0x220AAEE3:0x220AA920,0x220AAF1F:0x220AA920,
0x220AAF54:0x220AA920,0x220AAF75:0x220AA920,
}

# Previously authenticated Grunt logic calls the gate, not counted as new body.
CROSS_EDGES={
0x2235A5B0:0x22054D40,0x2235A760:0x22054D40,
0x2235A79D:0x22054D40,0x2235A97C:0x22054D40,
0x2235AA6B:0x22054D40,
}

# Representative *short* byte witnesses: data provenance, NOT a code dump.
WITNESSES={
0x22054CE9:'b9ecad5422', # global group registry
0x22054D13:'8b8ec4090000', # owner+0x9c4
0x22054D25:'c786c409000000000000', # clear linked group
0x22054D43:'83bec409000000', # null-group short circuit
0x22054D4E:'8d86cc090000', # context +0x9cc
0x22054D5B:'ff9254010000', # Grunt virtual +0x154
0x22054D61:'8b8ec4090000', # bit-group owner
0x22054D71:'33c0', # reject => false
0x22054D77:'b801000000', # absent group/accepted => true
0x22054D80:'8b81d0090000', # optional identity pointer at +0x9d0
0x22054D92:'3981ac030000', # compare candidate +0x3ac
0x22054DB4:'8b8fc4090000', # group pointer
0x22054DE2:'83be9400000000', # candidate +0x94
0x22054DF5:'c1e90b', # candidate +0xb8 bit 0x800
0x22054E23:'8b8fc4090000', # next iteration
0x22054E70:'8bcf', # owner fallback
0x22054E82:'8b500c', # metric object +0x0c
0x22054EA1:'6a01', # filtered enumeration
0x22054ECD:'3b4c2414', # scalar equality
0x220AAEB0:'8b442410', # first read fourth argument pointer
0x220AAEC0:'3bc6', # existing chosen index lower check
0x220AAEC7:'3bc5', # existing chosen index upper check
0x220AAEF3:'83e11f', # bit position mask
0x220AAF17:'83fdff', # unassigned old index sentinel
0x220AAF6E:'f7d2', # clear previously assigned bit
0x220AAF90:'5f', # preserve return/control branch
0x220AAF97:'8930', # update chosen index
0x220A9304:'807c241000', # filter boolean
0x220A930D:'397b48', # group count offset
0x220A9342:'83be0402000000', # entity state +0x204
0x220A9362:'ff9294060000', # virtual +0x694
0x220A9381:'8938', # output cursor
0x220A93C6:'830601', # increment passed cursor
0x220A93C9:'807c241000', # filter boolean
0x220A940C:'83bf0402000000', # entity state
0x220A942C:'ff9294060000', # virtual +0x694
}

def edge(pe,site,target):
    data=pe.read(site,5)
    if data[0]!=0xe8 or site+5+struct.unpack('<i',data[1:])[0]!=target:
        raise AssertionError(f'modified E8 edge at {site:#x} -> {target:#x}')

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'wrong original client.dll'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'wrong original server.dll'
    pe=PE32(server)
    assert pe.base==0x22000000
    spans=[]
    for va,(size,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text', f'{label} not in native .text'
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==sha,f'{label} SHA changed'
        spans.append((va,va+size))
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:]))
    # The methods 0x22054D40 and 0x220AAEB0 are ADJACENT to the following
    # functions and have NO trailing 0xCC alignment. Verify their true end
    # through complete method SHA, terminal RET and next entry witnesses.
    for va,size,terminal,after in [
       (0x22054D40,64,b'\xc2\x04\x00',b'\x8b\x81'),
       (0x220AAEB0,240,b'\xc2\x10\x00',b'\x53\x55')]:
        assert pe.read(va+size-3,3)==terminal
        assert pe.read(va+size,2)==after
    for va,(n,_,_) in METHODS.items():
        if va not in (0x22054D40,0x220AAEB0):
            assert pe.read(va+n,1)==b'\xcc',f'missing actual INT3 at {va+n:#x}'
    for site,target in {**CALLS,**CROSS_EDGES}.items():edge(pe,site,target)
    for site,witness in WITNESSES.items():
        assert pe.read(site,len(bytes.fromhex(witness)))==bytes.fromhex(witness),f'witness {site:#x} modified'
    assert sum(n for n,_,_ in METHODS.values())==1124
    return dict(new_complete_methods=len(METHODS),authenticated_original_server_bytes=1124,
                direct_calls=len(CALLS),prior_combat_crosscalls=len(CROSS_EDGES),
                opcode_witnesses=len(WITNESSES),no_cc_adjacent_boundaries=2)

def self_test():
    class Stub:
        def __init__(self,addr,payload): self.addr,self.payload=addr,payload
        def read(self,a,n):return self.payload[a-self.addr:a-self.addr+n]
    for site,target in {**CALLS,**CROSS_EDGES}.items():
        good=b'\xe8'+struct.pack('<i',target-site-5)
        edge(Stub(site,good),site,target)
        for i in range(5):
            bad=bytearray(good);bad[i]^=1
            try:edge(Stub(site,bad),site,target)
            except AssertionError: pass
            else:raise AssertionError('mutant E8 edge accepted')
    try:audit(b'MZinvalid',b'MZinvalid')
    except AssertionError:pass
    else:raise AssertionError('untrusted DLL pair accepted')
    return 'SELF_TEST_OK'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bin-zip',type=Path)
    ap.add_argument('--self-test',action='store_true')
    args=ap.parse_args()
    if not (args.bin_zip or args.self_test):ap.error('specify --bin-zip or --self-test')
    if args.self_test: print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            report=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_ORIGINAL_GRUNT_GROUP_PHASE560_AUTHENTICATED',report)
if __name__=='__main__':main()