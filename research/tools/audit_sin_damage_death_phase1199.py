#!/usr/bin/env python3
"""Authenticate Phase1196–1198 source boundaries without distributing DLLs."""
import argparse
import hashlib
import struct
import zipfile

CLIENT = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
SPANS = {
    0x22040140: (1144, 'f0e9c43ade8bf0689d094da29c117e29ba787169b62ea7d4e460e4c7b63b5114'),
    0x220a7ca0: (36, '14c9bc39f419a56001a280870e1f9efaadd8ffd0dd3668a9f82b9f088e499b1c'),
    0x22139f60: (19, 'd5ea9253030765bcd70227e7860e4620758e52644047e564b5756b5d78eaef44'),
    0x22042e70: (398, '6da0c7752de31a889b4a6b820886609aa7b429a436fbca864bbae6d378b2a4ca'),
    0x22355410: (99, '1455ece99bc11b7732f690300b086aaf857dc7dd9b02b7feb10c46f683db92e9'),
    0x22363550: (34, '6e427d23053c3bee61b8c116f7699a96a24aef385fdff72bbb65e17df0cf49c7'),
    0x22044730: (5, 'cc0e3cb106eb0fdec984d44a563d98c3da80700bbb8d2be4e66ebd54a9919626'),
    0x220eedb0: (34, '398c400f853d05d5b3c3059feb7778917f74793a6f76876b715ed932b2bebdbf'),
    0x220ecdd0: (8, 'b8a4d55dbd3c3148541815a558b937fef7d25f9ab18b899ebb066f74166b623b'),
    0x22027d90: (3, '1d10894625976bca9d8906f3ecf1e766b94e9219ec3bf1c30e02feea5e18e45a'),
    0x22001880: (1, 'ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e'),
}
SUBSPANS = [
    (0x22040140, 148, 'ec3397e99f2ae04ca5825c0283b0df3ccdad4142355228d076cadda733c7c98f'),
    (0x22042e70, 151, '2816574cf19967407840b43ff46b4b8ccc7aad75083bd0ba43cec3d13df1310b'),
    (0x22042f07, 247, 'ee1fd047214f4468dbebedd7a6855db29c866cbdc702a078091d072a743c0a18'),
]
GROUND = (0x224954a4, 0x2249aaa4, 0x224a416c)
ELITE = (0x2248df04,)
JET = (0x2248c514, 0x2248e83c, 0x2249964c, 0x224a4a4c)
COMMON = {0xf4:0x22044210, 0x370:0x22359ea0, 0x398:0x22044730,
          0x3f4:0x2216e6a0, 0x598:0x2235ae90, 0x1a8:0x220448d0,
          0x2a8:0x220eedb0, 0x2a4:0x220ecdd0, 0x4d0:0x22027d90}
EDGES = {
    0x22040163:(0xe8,0x222a7b20), 0x220401c1:(0xe8,0x220a7ca0),
    0x220a7ca7:(0xe8,0x22139f60), 0x22359ff2:(0xe8,0x22040140),
    0x22042e77:(0xe8,0x2205eec0), 0x22042e99:(0xe8,0x22042dc0),
    0x22042f29:(0xe8,0x222a7850), 0x22042f3d:(0xe8,0x22073700),
    0x22042ff5:(0xe8,0x22143d90), 0x22363553:(0xe8,0x221b9770),
    0x22355464:(0xe9,0x22315450),
}
VCALLS = {0x22040184:0x4d8,0x220401ce:0x598,0x22139f65:0xf4,
          0x22042e8c:0x4d8,0x22042eab:0x1a8,0x22042eed:0x3f4,
          0x22042ef7:0x180,0x22042f01:0x58c,0x22042f15:0x398,
          0x22042f32:0x760,0x22042f81:0x2a8,0x22042f8f:0x2a4,
          0x22042fd8:0x4d0}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def reader(data):
    assert data[:2] == b'MZ'
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe+4] == b'PE\0\0'
    machine, count = struct.unpack_from('<HH', data, pe+4)
    optlen = struct.unpack_from('<H', data, pe+20)[0]
    opt = pe+24
    assert machine == 0x14c and struct.unpack_from('<H', data, opt)[0] == 0x10b
    base = struct.unpack_from('<I', data, opt+28)[0]
    assert base == 0x22000000
    sections = []
    for i in range(count):
        off = opt+optlen+40*i
        name = data[off:off+8].rstrip(b'\0')
        _, va, rawsize, raw = struct.unpack_from('<IIII', data, off+8)
        sections.append((name, base+va, rawsize, raw))
    def read(addr, size, section=None):
        for name, start, length, raw in sections:
            if start <= addr and addr+size <= start+length:
                assert section is None or section == name
                off = raw+addr-start
                assert off+size <= len(data)
                return data[off:off+size]
        raise AssertionError(f'Unmapped VA {addr:#x}')
    return read


def audit(client, server):
    assert sha(client) == CLIENT and sha(server) == SERVER, 'Unsupported original pair'
    read = reader(server)
    for va, (n, expected) in SPANS.items():
        assert sha(read(va,n,b'.text')) == expected, hex(va)
        assert read(va+n,1,b'.text') == b'\xcc', hex(va)
    for va,n,expected in SUBSPANS:
        assert sha(read(va,n,b'.text')) == expected, hex(va)
    for site,(op,target) in EDGES.items():
        ins = read(site,5,b'.text')
        assert ins[0] == op and site+5+struct.unpack_from('<i',ins,1)[0] == target, hex(site)
    for site,slot in VCALLS.items():
        ins = read(site,6,b'.text')
        assert ins[:2] in (b'\xff\x90',b'\xff\x92') and struct.unpack_from('<I',ins,2)[0] == slot, hex(site)
    for vt in GROUND+ELITE+JET:
        slots = dict(COMMON)
        slots[0x180] = 0x22363550 if vt in JET else 0x22355410 if vt in ELITE else 0x22001880
        slots[0x58c] = 0x22363af0 if vt in JET else 0x2235aff0
        slots[0x4d8] = 0x22366e60 if vt in JET else 0x22040ea0
        for slot,target in slots.items():
            assert struct.unpack('<I',read(vt+slot,4,b'.rdata'))[0] == target, (hex(vt),hex(slot))
    return dict(full_hash_spans=len(SPANS), bounded_spans=len(SUBSPANS), direct_edges=len(EDGES), virtual_edges=len(VCALLS), vtable_slots=8*(len(COMMON)+3))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',required=True)
    a = p.parse_args()
    with zipfile.ZipFile(a.bin_zip) as z:
        client,server = z.read('client.dll'),z.read('server.dll')
    print('SIN_DAMAGE_DEATH_AUTHENTICATED',audit(client,server))
    for original, index in ((client,0),(server,1)):
        bad = bytearray(original); bad[-1] ^= 1
        pair = [client,server]; pair[index] = bad
        try: audit(*pair)
        except AssertionError: pass
        else: raise AssertionError('Mutated original accepted')
    print('MUTATION_REJECTION_PASS 2/2')

if __name__ == '__main__': main()
