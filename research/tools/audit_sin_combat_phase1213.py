#!/usr/bin/env python3
"""Authenticate original combat reconstruction evidence, phases 1200–1212."""
import argparse
import struct
import zipfile
from audit_sin_damage_death_phase1199 import audit as prior_audit, reader, sha, GROUND, ELITE, JET

# Final byte after each span is CC except for the middle damage block.
SPANS = [
    (0x2216e510,104,'3fd8b4e4642526aa8171927faf9be753b3d7761ec7e2c48a49e73786eafdd216'),
    (0x22042dc0,161,'0431aaaa9057c052986ff450561fcd49ab6e0f0fde5aff793dd0be9da5aa2a3b'),
    (0x22042b60,65,'6b796901e5ada7fe8ca83eefc33306a05861209ef1ec89464d5e09f3fb96e57d'),
    (0x22042bb0,65,'0640cc9fc51874b926d196af6d012758ef7a956f3ed32d7a0bf82c80ee454254'),
    (0x22040ea0,193,'5cd9de00d34cd9fe562ade774987e44945ffe3592f724639954c6ad1c0cfeaa2'),
    (0x22366e60,100,'72858b7888f9d76fb0eaf1dc073b5038a366e04d6eb191c7a9c9fb5a330e6926'),
    (0x220401d4,283,'643d3bc98629e81c4a4fa843fbd0022de5f1170a959ec19b82f20493794afc6a'),
    (0x220402ef,713,'30ea895b1d785a1837a8d969652b12b5d8c7b2c1259edbbd904e24317d36c841'),
    (0x220381f0,54,'86a5f5bb37296834e016594b6f96ed41073cd74f90ce66c9222bc082d7e47621'),
    (0x22359ea0,586,'dff7b5b2cfe8422dffa2c51d78e2a4cb81f622b155bd3998926e9a56387a79fd'),
    (0x22040610,257,'e760abf0e94dc47ffe81a7893589f86cf8f0e6992e8e8d13d7859cc425d2900d'),
    (0x22033e10,249,'238f18783db3e165e675148bf59332510ae90aac9c41faaed3b07b6c2c4f237b'),
    (0x22358490,165,'212a14a2dad0a5caf50dbd530c51721871034f726cb29aa630d51041721a1b42'),
    (0x223594a0,252,'57b33745a7e3d2f4a96771a832a94a7d7f868ecde3cfdf695e4c67ec90fd2b42'),
    (0x22358590,19,'010287fed98c2f063bf7d10504dfca1c1dd6e1cec5595f1411a5f6b6676ab820'),
    (0x22030520,238,'bcf586a052ee52d9665ac89559a60b24b2ffa7e6541392f1875fa2ec96483a60'),
    (0x22119b70,255,'3dc529bd0a766b7fd212351b993606216948954d9dccc25f50904886831d6235'),
    (0x22119c70,168,'5ecf0d42440232eb8536d936ce466709e3a31a2fc11b1820336166df10100763'),
    (0x22141910,127,'406695d39629e60c5c899fc6239694e06bb05ccd3fc13b3372c62b72d932d8f2'),
]
EDGES = [
    (0x22042dfa,0xe8,0x22119c70),(0x22119c9c,0xe8,0x22119b70),
    (0x2235859e,0xe9,0x22030520),(0x22040618,0xe8,0x22139f60),
    (0x22366e6c,0xe8,0x22366a20),(0x22366ebb,0xe8,0x22040ea0),
    (0x22141938,0xe8,0x22268fd0),(0x2214194c,0xe8,0x22268fd0),
    (0x220305b2,0xe8,0x2206bba0),(0x220305f6,0xe8,0x220a9680),
]
SLOTS = {
    0x1dc:0x22141910,0x370:0x22359ea0,0x37c:0x22033e10,0x380:0x22040610,
    0x670:0x22030d10,0x678:0x22358590,0x67c:0x220304f0,0x740:0x2202f2c0,
    0x744:0x22358ed0,0x778:0x2216e310,0x7a8:0x22357f10,0x7d0:0x22354010,
    0x7d4:0x22354020,0x7e4:0x22354060,0x834:0x223541a0,
}
JUMP = (0x22119cbf,0x22119cdd,0x22119cdd,0x22119cc9,0x22119cd3,0x22119ce7,0x22119cf1)

def audit(client,server):
    prior_audit(client,server)
    read=reader(server)
    for va,n,digest in SPANS:
        assert sha(read(va,n,b'.text'))==digest,hex(va)
        expected=0xf6 if va==0x220401d4 else 0xcc
        assert read(va+n,1,b'.text')[0]==expected,hex(va+n)
    for site,op,target in EDGES:
        ins=read(site,5,b'.text')
        assert ins[0]==op and site+5+struct.unpack_from('<i',ins,1)[0]==target,hex(site)
    for vt in GROUND+ELITE+JET:
        slots=dict(SLOTS)
        slots[0x464]=0x22355d90 if vt in ELITE else 0x2235ef10
        slots[0x4d8]=0x22366e60 if vt in JET else 0x22040ea0
        slots[0x7a4]=0x22355da0 if vt in ELITE else 0x22353f50
        for slot,expected in slots.items():
            assert struct.unpack('<I',read(vt+slot,4,b'.rdata'))[0]==expected,(hex(vt),hex(slot))
    assert struct.unpack('<7I',read(0x22119cfc,28,b'.text'))==JUMP
    assert struct.unpack('<f',read(0x223d2590,4))[0]==0
    assert struct.unpack('<f',read(0x223d8fe4,4))[0]==-1
    return dict(spans=len(SPANS),edges=len(EDGES),slots=8*(len(SLOTS)+3),jump_entries=len(JUMP),constants=2)

def main():
    if not __debug__:
        raise SystemExit('Run without Python -O')
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',required=True)
    with zipfile.ZipFile(p.parse_args().bin_zip) as z:
        c,s=z.read('client.dll'),z.read('server.dll')
    print('SIN_COMBAT_ORIGINAL_AUTHENTICATED',audit(c,s))

if __name__=='__main__':
    main()
