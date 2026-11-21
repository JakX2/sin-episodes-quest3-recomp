#!/usr/bin/env python3
"""Owner-private authenticity check for original Grunt sidestep gate +0x848.

Requires the same exact 2006 retail game DLL pair. Prints only bounded
structural metadata, never original code or commercial binaries.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
GATE_VA=0x22359620
SELECTOR_VA=0x2235E4E0
GATE_LENGTH=164
SELECTOR_LENGTH=991
GATE_SHA='4eab1c761ef9d125b01708a37c919bec1b4042f3d99a834a16243a8f23364f3f'
SELECTOR_SHA='05b949835363d8576fbc9839747474575c20e408c0c3a2be8ac9ec4e1d89acfd'

class Refusal(ValueError):
    pass

def require(ok,msg):
    if not ok:
        raise Refusal(msg)

class PE:
    def __init__(self,raw):
        self.raw=raw
        require(raw[:2]==b'MZ','missing MZ header')
        header=struct.unpack_from('<I',raw,0x3C)[0]
        require(raw[header:header+4]==b'PE\0\0','PE header')
        coff=header+4
        require(struct.unpack_from('<H',raw,coff)[0]==0x14C,'not Intel PE32')
        num=struct.unpack_from('<H',raw,coff+2)[0]
        optlen=struct.unpack_from('<H',raw,coff+16)[0]
        opt=coff+20
        require(struct.unpack_from('<H',raw,opt)[0]==0x10B,'wrong optional header type')
        self.base=struct.unpack_from('<I',raw,opt+28)[0]
        self.sections=[]
        for i in range(num):
            p=opt+optlen+i*40
            name=raw[p:p+8].split(b'\0')[0].decode('ascii','replace')
            virtual,va,rawlen,offset=struct.unpack_from('<IIII',raw,p+8)
            require(offset+rawlen<=len(raw),'section out of bounds')
            self.sections.append((name,self.base+va,min(virtual,rawlen),offset))
    def at(self,va,n):
        for name,start,count,off in self.sections:
            if start<=va and va+n<=start+count:
                return self.raw[off+va-start:off+va-start+n]
        raise Refusal('unmapped original RVA '+hex(va))
    def u32(self,va):return struct.unpack('<I',self.at(va,4))[0]
    def section(self,va):
        for name,start,count,off in self.sections:
            if start<=va<start+count:return name
        return None

def verify_call(read,site,destination):
    blob=read(site,5)
    require(len(blob)==5 and blob[0]==0xE8,'not original CALL '+hex(site))
    displacement=struct.unpack_from('<i',blob,1)[0]
    require(site+5+displacement==destination,'CALL destination mismatch '+hex(site))

def verify_original(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'2006 server.dll SHA mismatch')
    require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'2006 client.dll SHA mismatch')
    pe=PE(server)
    require(pe.base==0x22000000,'server.dll image base mismatch')
    for va,length,digest in ((GATE_VA,GATE_LENGTH,GATE_SHA),
                             (SELECTOR_VA,SELECTOR_LENGTH,SELECTOR_SHA)):
        require(pe.section(va)=='.text','not in original text section')
        require(hashlib.sha256(pe.at(va,length)).hexdigest()==digest,
                'complete method SHA changed '+hex(va))
    require(pe.u32(0x2249AAA4+0x848)==GATE_VA,
            'Grunt vslot +0x848 no longer targets verified gate')
    checks=(
        (0x22359623,b'\x80\xbe\xa0\x0c\x00\x00\x00','byte +0xCA0'),
        (0x22359630,b'\xf6\x86\x88\x09\x00\x00\x02','flag +0x988'),
        (0x2235963D,b'\x6a\x04','condition 4'),
        (0x22359648,b'\x6a\x03','condition 3'),
        (0x22359655,b'\x83\xbe\x8c\x00\x00\x00\x00','field +0x8C'),
        (0x22359670,b'\xff\x97\x04\x03\x00\x00','Grunt virtual +0x304'),
        (0x22359685,b'\xff\x92\x00\x03\x00\x00','manager virtual +0x300'),
        (0x2235968F,b'\x6a\x4d','condition IN_PLAYER_SIGHTS 0x4D'),
        (0x223596A4,b'\x68\x00\x00\x80\x40','factor 4.0f'),
        (0x223596B3,b'\x68\x00\x00\x80\x3f','factor 1.0f'),
        (0x223596A9,b'\xff\x90\xa0\x01\x00\x00','rules vslot +0x1A0 sighted'),
        (0x223596B8,b'\xff\x92\xa0\x01\x00\x00','rules vslot +0x1A0 alternate'),
        (0x2235969A,b'\x8b\x0d'+struct.pack('<I',0x22572914),'original mobility global'),
        (0x2235E79E,b'\x8b\x86\x50\x08\x00\x00','selector field +0x850'),
        (0x2235E7BB,b'\xff\x92\x48\x08\x00\x00','selector +0x848 call'),
        (0x2235E7C8,b'\xb8\x55\x00\x00\x00','selector raw return schedule 0x55'),
        (0x2235E7D3,b'\xff\x90\x44\x08\x00\x00','selector +0x844 fallback'),
        (0x223600C1,b'\xc7\x44\x24\x10'+struct.pack('<I',0x2249356C),
         'native sidestep schedule registration string pointer'),
        (0x223600C9,b'\xc7\x44\x24\x14'+struct.pack('<I',0x55),
         'native sidestep schedule registration numeric ID'),
    )
    for va,expected,label in checks:
        require(pe.at(va,len(expected))==expected,'witness changed: '+label)
    calls=((0x2235963F,0x220397C0),(0x2235964C,0x220397C0),
           (0x22359661,0x2202E310),(0x22359668,0x220092D0),
           (0x2235967B,0x2202E310),(0x22359693,0x220397C0),
           (0x223600D7,0x2208C590))
    for site,destination in calls:verify_call(pe.at,site,destination)
    registered_name=b'SCHED_GRUNT_MOBILITY_SIDESTEP\0'
    require(pe.at(0x2249356C,len(registered_name))==registered_name,
            'registered schedule name does not match original retail ID 0x55')
    return {
        'status':'ORIGINAL_GRUNT_SIDESTEP_GATE_AUTHENTICATED',
        'original_server_sha256':SERVER_SHA,
        'complete_original_methods_verified':2,
        'verified_complete_method_bytes':GATE_LENGTH+SELECTOR_LENGTH,
        'gate_virtual_offset':'0x848',
        'state_three_field':'0x850',
        'successful_state_three_raw_schedule_id':'0x55',
        'verified_registered_schedule_name':'SCHED_GRUNT_MOBILITY_SIDESTEP',
        'fallback_grunt_virtual_offset':'0x844',
        'original_direct_calls_verified':len(calls),
        'exact_instruction_witnesses_verified':len(checks),
        'sight_condition':'0x4D (COND_GRUNT_IN_PLAYER_SIGHTS)',
        'original_rules_mobility_factors':[4.0,1.0],
        'limit':'Original schedule name/ID registration authenticated; no independent engine boot.'
    }

def self_test():
    assert GATE_LENGTH+SELECTOR_LENGTH==1155
    assert len(GATE_SHA)==len(SELECTOR_SHA)==64
    assert [4,3,0x4D]==[4,3,77]
    a,b=0x10001000,0x10002000
    native=b'\xE8'+struct.pack('<i',b-a-5)
    verify_call(lambda address,count:native[:count],a,b)
    for i in range(5):
        mut=bytearray(native);mut[i]^=1
        try:verify_call(lambda address,count:bytes(mut[:count]),a,b)
        except Refusal:pass
        else:raise AssertionError('mutation accepted at byte '+str(i))
    try:verify_original(b'not a game dll',b'not a client dll')
    except Refusal:pass
    else:raise AssertionError('bad original accepted')
    print('PASS original Grunt sidestep audit self-test: six call cases, SHA refusal')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-zip',type=Path)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:self_test()
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            originals={Path(name).name.lower():z.read(name)
                       for name in z.namelist()
                       if Path(name).name.lower() in ('server.dll','client.dll')}
        require(set(originals)=={'server.dll','client.dll'},'missing originals in ZIP')
        print(json.dumps(verify_original(originals['server.dll'],originals['client.dll']),indent=2))
    if not args.self_test and not args.bin_zip:parser.error('provide --self-test or --bin-zip')

if __name__=='__main__':
    main()