#!/usr/bin/env python3
"""Owner-local evidence audit of original SiN life/death and activity dispatch."""
import argparse
import hashlib
import struct
import zipfile

SPANS = [
    (0x222a7b20,645,'457841af1bb0cfe7c486f1641fea48b816cd35190b3f40ef4df3f8e1323b1548',0xcc),
    (0x220392f0,126,'5243a212fb24abecba7d862ee49aa0591c4fbb34b470ffa749b788e9a2a463b5',0xcc),
    (0x22039370,510,'8d354cdc37eeeea5cdf48e3aada124fe0698c543e19724a472444d43e60ecbcd',0xcc),
    (0x223595a0,117,'02d7a59f28196c80c4643e764cb48c7894de1a20615b7812cdde7a90b18357d5',0xcc),
    (0x22353da0,20,'cfacf533f2ca33fd67165c6c8d72455078b2e8ef44fa1ac0e87808a6fffbfb9d',0xcc),
    (0x220f2210,230,'4840cda3a8af9a6b30ea6679dcb381fc76dbe5d88dc93b7659fcf60b71960917',0xcc),
    (0x222a57c0,527,'e46ae67ca20cd09fbbd770fb7cf8c037c9f77f5acb45afad10e3209b5af6de71',0xcc),
    (0x22366730,239,'4ed80bec768952ac1096ecb169b34740eae8d18f4571beaa21a4e919c4498f4c',0x8b),
    (0x2220dfc0,115,'c00cbf0fd40c3dcd310d1eed01683aed66414c32e17c0d43ede7a73016fc5751',0xcc),
    (0x2235e0b0,1063,'178f529655a3f8b64263476fd0a72898e5a5741c5963c01817d16a623c765579',0xcc),
    (0x2208b3b0,59,'5b6ec053988b497c50b9048d2072a56260fd740bd9758edf19c3c184d8b5940d',0xcc),
    (0x2235ef10,404,'1fbe5299da4ed43f32c4c9c753d3e6ee41eb27f2ff0316ea4d7655eefcb902e5',0xbf),
    (0x2235be70,99,'af9a74dc65a3d63de1c2ef4b2c8d0b0db54718aa5553b15792a1e5d37250f587',0xcc),
    (0x22358a60,125,'4d3ae5989cbd784b7188d2c7698c4601c63e9a96d06c14f65329dc3dccd95b0c',0xcc),
    (0x2236681f,205,'c61b379a4839e802bdff8e2975fe181aa99efb198ef8990dfebc258c36795db5',0x8b),
    (0x223668ec,280,'1d2212339fea0f598e568aa6e6e9d47425c6f86e123c75f360481208a0456438',0x26),
    (0x22365760,1249,'a455aba89fb336cf5776fadf48b83124bf68f09d4541648f1b6d0aab7d058459',0xcc),
    (0x22354fb0,741,'254d02c28a4201e974d5113627ee44e2e64e2295e051a5a8626d6c775302d762',0xcc),
    (0x22354e20,224,'4ff16cd6a842e47e81de35372cdc9ee73c4b3cca141954a5db556150951130d1',0xa0),
    (0x2235a250,326,'aacea02b9e14ff6ba7a060bbbac75dd33bca28db374218029b1cb0f8e91d5351',0xcc),
    (0x22364700,274,'b9d06b726a6036fba8a32cafc3bcb4b5c1d762483b7d26aae226a66a7a113173',0xcc),
    (0x2203cfc0,170,'a6898e05cf04d849b2c110e66f2878e5b4565d3b55d13b02f6452521562c5e56',0xcc),
    (0x22030360,43,'82f9a05d0641492689f5c9c20b17a6a89065350941234b5d5196b441bf5c1098',0xcc),
    (0x22030e50,49,'6246ada01c067b4a8bddc9dd98c18e67d405508c0f384fd90962d1be82983dc2',0xcc),
]
VTS = (0x2248df04,0x224954a4,0x2249aaa4,0x224a416c,0x2248c514,0x2248e83c,0x2249964c,0x224a4a4c)
COMMON = {0x370:0x22359ea0,0x374:0x220392f0,0x378:0x22039370,0x398:0x22044730,0x3a8:0x222a4490,0x1b4:0x220018f0,0x1c4:0x22358a60,0x714:0x22030e50,0x718:0x2203da40,0x5c0:0x22030360}
EDGES = [(0x22365c01,0x2214d6f0),(0x22365c19,0x2235e0b0),(0x22365c37,0x2214d7b0),(0x22355272,0x2235e0b0),(0x223647b7,0x2235a250),(0x2235a38c,0x2203cfc0),(0x2203d041,0x22039130),(0x2203d054,0x220eebd0)]

def require(ok, label):
    if not ok:
        raise ValueError(label)

def audit(client, server):
    sha=lambda v: hashlib.sha256(v).hexdigest()
    require(sha(client)=='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d','client identity')
    require(sha(server)=='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650','server identity')
    # These authenticated owner binaries have matching virtual/raw offsets in .text and .rdata.
    def read(va,n,lo=0x22001000,hi=0x223d111a):
        require(lo<=va and va+n<=hi,'section bounds '+hex(va))
        return server[va-0x22000000:va-0x22000000+n]
    for va,n,digest,next_byte in SPANS:
        require(sha(read(va,n))==digest,'span '+hex(va))
        require(read(va+n,1)==bytes([next_byte]),'boundary '+hex(va+n))
    for site,target in EDGES:
        op=read(site,5)
        require(op[0]==0xe8 and site+5+struct.unpack_from('<i',op,1)[0]==target,'edge '+hex(site))
    slots=0
    for i,vt in enumerate(VTS):
        jet=i>=4
        expected=dict(COMMON)
        expected.update({0xe8:0x22366730 if jet else 0x22354fb0 if i==0 else 0x2235e0b0,0x338:0x22364700 if jet else 0x2235a250,0x3ac:0x223654b0 if jet else 0x2235a140,0x464:0x22355d90 if i==0 else 0x2235ef10,0x2d4:0x22353da0 if i==4 else 0x223595a0})
        if i==0: expected[0x850]=0x22354e20
        for offset,va in expected.items():
            require(struct.unpack('<I',read(vt+offset,4,0x223d2000,0x224d5626))[0]==va,f'vtable {vt:x}+{offset:x}')
            slots+=1
    require(struct.unpack('<6I',read(0x22366a04,24))==(0x22366826,0x22366826,0x223669fb,0x223668ec,0x2236681f,0x223668ec),'death jump table')
    require(struct.unpack('<6I',read(0x2236573c,24))==(0x22365511,0x22365511,0x22365725,0x22365725,0x22365511,0x22365725),'ragdoll jump table')
    require(struct.unpack('<3I',read(0x223647c4,12))==(0x2236478f,0x22364785,0x223647b4),'activity jump table')
    expected=bytes(0 if i in (1,7,16) else 1 if i==66 else 2 for i in range(1,67))
    require(read(0x223647d0,66)==expected,'activity byte table')
    for va,bits in [(0x223d9cc0,0x40000000),(0x223d9788,0x3dcccccd),(0x223d5830,0x42480000),(0x223f0408,0x40e00000)]:
        require(struct.unpack('<I',read(va,4,0x223d2000,0x224d5626))[0]==bits,'constant '+hex(va))
    return dict(spans=len(SPANS),boundaries=len(SPANS),edges=len(EDGES),slots=slots,table_entries=81,constants=4)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',required=True)
    with zipfile.ZipFile(p.parse_args().bin_zip) as z:
        client,server=z.read('client.dll'),z.read('server.dll')
    print('SIN_LIFE_ORIGINAL_AUTHENTICATED',audit(client,server))
    for a,b in [(client[:-1]+bytes([client[-1]^1]),server),(client,server[:-1]+bytes([server[-1]^1]))]:
        try: audit(a,b)
        except ValueError: pass
        else: raise RuntimeError('mutated input admitted')
    print('MUTATED_INPUT_REJECTIONS 2/2')
