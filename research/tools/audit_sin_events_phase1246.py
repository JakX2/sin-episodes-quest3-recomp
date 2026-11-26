#!/usr/bin/env python3
"""Authenticate the owner-local event reconstruction evidence; never export DLL bytes."""
import argparse
import hashlib
import struct
import zipfile

CLIENT = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
SPANS = (
    (0x2202d930,72,'969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2',0xcc),
    (0x2202d890,147,'69f8cf1d37b414a3c0e2f5db80c61df62d52d6198e03d62fa4ef18903e462d4b',0xcc),
    (0x220cd2f0,7,'1823d20cb51f1dda02bb824b26fb3f0b5d377c30ac6c844f2419d5cd71db5186',0xcc),
    (0x220dea60,387,'eff73b12af6f7b712a89cc88d56a1f27fd12cddb9e8b22cc6b2dbe68a1ec3446',0xcc),
    (0x22392970,1128,'fed2eda6f89634f075fd61cda6189a04f0dec3675c7a64bcf98ced101f3cdf3b',0xcc),
    (0x22399650,840,'db6b11082b545915b8cc97007056bed4fe71a5f362731d00cfa1f25f63545d29',0xcc),
    (0x220e0290,738,'dd6a9103d73cd60fa0c0ab87219dd860f08a9a02467d5ecb382dfe793953b5cc',0xcc),
    (0x22392790,60,'55c24daf3da453809e6e0692d604c1a7008273d6a08b2ec9f38c4bbc094c8f0a',0xcc),
    (0x22392670,33,'a955fa91e654e2d72c4c1b7a9678140e617aeab199dd746de2b2173017757522',0xcc),
    (0x22027ca0,60,'a6ad19ca3a57bed3a25fdcddaae6a80dbac81ae489f7bba36a39ec52167e04ab',0xcc),
    (0x22039130,46,'c36aee349e16e9d0b69892b640980067cee17aaca0f14c4d55dada9110bc8c6c',0xcc),
    (0x220d0d20,107,'509e14b51a86dbd18544b9c0c8d3bc4669fc95a607a9e7fadf2f506fc7be5709',0xcc),
    (0x220df780,53,'4cb7a73a686972257dbf14af46252d25478d06dd58b5b6c88f08368631ec6d3e',0xcc),
    (0x2204e7e0,70,'35c04c025de6fce64ce7fc33a50d9a2164be56d91ceda083121086f75ca2b5fb',0xcc),
    (0x220e01a0,230,'cd4236cc1836ff51b81b507017808ea6d7b52f93ba3dc4bbfc5c485cfe3571f7',0xcc),
    (0x22009210,60,'9de1466d688be95d555054609c541fdc11db9a8d48b3814e784b3eebe0f5de10',0xcc),
    (0x22041a60,128,'b2999894cf09b0e3bf4593899e6b8ccc9dbfc7d7f0bf774f86fae4013e97f3a0',0x83),
    (0x220b1c30,125,'8b86ba95c8d6172c833ba16990177db7e421f1a29172079d823d7919f204cbbb',0xcc),
    (0x22030930,155,'26295217e98713fcfd64b8798f97ed5a0624e5ef5c0607c3ce829673f02b6467',0xcc),
    (0x2235ae30,85,'d4cf7a90c31cc3f6d7fb9412e5570cf975bbf130c9b70967bc4a7dcce32a68a1',0xcc),
    (0x2203d2d0,93,'045a0f4fa001ae8c01ae319839067cec5aed1facaa6452174c9cda2d6eece679',0xcc),
)
ACTORS = (0x2248df04,0x224954a4,0x2249aaa4,0x224a416c,0x2248c514,0x2248e83c,0x2249964c,0x224a4a4c)
COMMON = {0x154:0x22044840,0x3c0:0x223585b0,0x6b4:0x220446b0,0x780:0x2202d890,0x5a8:0x2235ae30,0x680:0x22041a60,0x684:0x22030930}
BEHAVIORS = {0x223f956c:0x220dea60,0x224abec4:0x22392970,0x224ae624:0x22399650,0x22495344:0x220e0290}
TABLES = {0x22392dbc:(0x223929d1,0x22392a9a,0x22392b81,0x22392bb5,0x22392c6b,0x22392d85,0x22392999),0x22399980:(0x2239971e,0x2239982d,0x22399940,0x22399955,0x223996e5,0x22399678)}
CONSTANTS = {0x223d8f14:0x41200000,0x223d9cac:0x41800000,0x223da014:0x42400000,0x223da4a8:0x40a00000,0x223d8fe0:0x41f00000,0x223d22f8:0x3f800000,0x223daf54:0x44800000,0x223d8e0c:0x43360b61,0x223d8e08:0x3bb40000}
REASONS = {0x223f7868:b'NO PATH',0x224ab994:b'Not correctly on func_tank man point',0x223eeb5c:b'No hint node',0x224ae350:b'No longer behind cover'}

def require(ok, label):
    if not ok:
        raise ValueError(label)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def reader(raw):
    p=struct.unpack_from('<I',raw,0x3c)[0]
    require(raw[:2]==b'MZ' and raw[p:p+4]==b'PE\0\0','PE signature')
    require(struct.unpack_from('<H',raw,p+4)[0]==0x14c and struct.unpack_from('<H',raw,p+24)[0]==0x10b,'PE32 x86')
    base=struct.unpack_from('<I',raw,p+52)[0]
    require(base==0x22000000,'preferred base')
    start=p+24+struct.unpack_from('<H',raw,p+20)[0]
    sections=[]
    for i in range(struct.unpack_from('<H',raw,p+6)[0]):
        vs,va,rs,off=struct.unpack_from('<IIII',raw,start+i*40+8)
        require(off+rs<=len(raw),'section extent')
        sections.append((base+va,min(vs or rs,rs),off))
    def read(va,n):
        for first,size,off in sections:
            if first<=va and va+n<=first+size:
                return raw[off+va-first:off+va-first+n]
        raise ValueError('unmapped original VA '+hex(va))
    return read

def evidence(read):
    u32=lambda va:struct.unpack('<I',read(va,4))[0]
    for va,n,digest,next_byte in SPANS:
        require(sha(read(va,n))==digest,'span '+hex(va))
        require(read(va+n,1)==bytes([next_byte]),'boundary '+hex(va+n))
    for vt in ACTORS:
        for slot,target in COMMON.items():
            require(u32(vt+slot)==target,'actor virtual '+hex(vt+slot))
    for vt,target in BEHAVIORS.items():
        require(u32(vt+0x60)==target,'behavior event '+hex(vt))
    for va,targets in TABLES.items():
        for i,target in enumerate(targets):
            require(u32(va+4*i)==target,'event table '+hex(va+4*i))
    for va,bits in CONSTANTS.items():
        require(u32(va)==bits,'constant '+hex(va))
    require(struct.unpack('<d',read(0x223d9700,8))[0]==-0.001,'Standoff timer threshold')
    for va,s in REASONS.items():
        require(read(va,len(s)+1)==s+b'\0','request reason '+hex(va))
    return dict(spans=len(SPANS),boundaries=len(SPANS),vtable_entries=len(ACTORS)*len(COMMON)+len(BEHAVIORS),dispatch_entries=sum(map(len,TABLES.values())),constants=len(CONSTANTS)+1,reasons=len(REASONS))

def audit(client,server):
    require(sha(client)==CLIENT,'client identity')
    require(sha(server)==SERVER,'server identity')
    return evidence(reader(server))

def negatives(client,server):
    def refuse(fn):
        try: fn()
        except ValueError: return 1
        raise AssertionError('mutated input admitted')
    n=refuse(lambda:audit(client[:-1]+bytes([client[-1]^1]),server))
    n+=refuse(lambda:audit(client,server[:-1]+bytes([server[-1]^1])))
    read=reader(server)
    for bad in (SPANS[0][0],SPANS[0][0]+SPANS[0][1],ACTORS[0]+0x154,0x22392dbc,0x223d8f14,0x223f7868):
        def corrupted(va,size):
            data=bytearray(read(va,size))
            if va<=bad<va+size:data[bad-va]^=1
            return bytes(data)
        n+=refuse(lambda:evidence(corrupted))
    return n

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',required=True)
    with zipfile.ZipFile(p.parse_args().bin_zip) as z:
        client,server=z.read('client.dll'),z.read('server.dll')
    print('SIN_BEHAVIOR_EVENT_EVIDENCE',audit(client,server))
    print('MUTATION_REJECTIONS',negatives(client,server))
