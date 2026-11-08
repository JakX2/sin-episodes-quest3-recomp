#!/usr/bin/env python3
"""Read-only SHA-pinned original SiN (2006) server.dll pose second-pass state audit.
Owner must supply the private original DLL; none of its bytes are exported.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'ActiveTrackEligibility':(0x22103D70,0x22103D8A,'4cb26bfb44bd84da46ff3207c83bdc1ae4fba066b87b5c0195efe67103e30279'),
 'TrackGeometryCalls':(0x22103DC3,0x22103E15,'6f4d9cbf642fe6bb1b2459d344d7397b9ecd05620d5e8af990f4a8997904034c'),
 'FullCapFlag':(0x22103E15,0x22103E3A,'092b4b1d178968c93cce23cb2baa40f600d39da10d86919f3d08a7d514d4c7f1'),
 'SequenceHistoryWindow':(0x22103EEA,0x22103F1C,'51d442754a0be1970c0234f0dac4adeac5dabbbb345c2fe4a433932d3397762b'),
 'TrackStrideAdvance':(0x221048A6,0x221048D5,'23d05658771f0375ca0b892c99330abd06259c7ed6b606a884c201f98fa1a302')
}
CALLS={
 0x22103DC8:0x22100F30,
 0x22103DDD:0x2226D250,
 0x22103DED:0x22268F10,
 0x22103E10:0x22269520,
 0x22103F2C:0x2226CA30,
}
ANCHORS={
 0x22103D70:'8b7500',      # track pointer
 0x22103D73:'d9443e5c',    # influence at +0x5C
 0x22103D79:'dc1d988f3d22', # compare 0.0
 0x22103D81:'f6c441',      # skip <=0 or unordered
 0x22103D84:'0f851c0b0000',
 0x22103E15:'d94644',      # track cap at +0x44
 0x22103E18:'dd05b8223d22', # load 1.0 constant
 0x22103E21:'dae9',        # FUCOMPP
 0x22103E25:'f6c444',      # parity C2/C3
 0x22103E28:'0f8ab5000000',
 0x22103E2E:'c686a000000001', # write cap marker=true
 0x22103EE3:'c686a000000000', # write cap marker=false
 0x22103EEA:'8b4570',      # sequence index parameter
 0x22103EF1:'8b8ea8000000',# prior index +0xA8
 0x22103EFA:'3bca',        # cmp last, current-1
 0x22103EFE:'3bc8',        # cmp last, current
 0x22103F02:'c686a100000000', # reset flag +0xA1
 0x22103F09:'c786a400000000000000', # clear accumulator +0xA4
 0x22103F13:'8b4570',
 0x22103F16:'8986a8000000',# store current index at +0xA8
 0x221048B1:'81c7e4000000' # track stride 0xE4
}
CONSTANTS={
 0x223D8F98:0.0, # 64-bit eligibility comparison
 0x223D22B8:1.0  # 64-bit cap marker comparison
}
def read(pe,address,size):
    fn=getattr(pe,'read',None)
    return fn(address,size) if callable(fn) else pe.read_va(address,size)
def call_target(pe,address):
    data=read(pe,address,5)
    if data[0]!=0xE8:raise ValueError('expected near CALL at '+hex(address))
    return address+5+struct.unpack('<i',data[1:])[0]
def verify(data):
    sha=hashlib.sha256(data).hexdigest()
    if sha!=SERVER_SHA:raise ValueError('owner original DLL hash mismatch')
    pe=PE32(data)
    if pe.image_base!=0x22000000:raise ValueError('unexpected original image base')
    for name,(start,end,digest) in REGIONS.items():
        if hashlib.sha256(read(pe,start,end-start)).hexdigest()!=digest:
            raise ValueError('code region mismatch: '+name)
    for at,expected in CALLS.items():
        if call_target(pe,at)!=expected:
            raise ValueError('CALL mismatch: '+hex(at))
    for at,encoded in ANCHORS.items():
        expected=bytes.fromhex(encoded)
        if read(pe,at,len(expected))!=expected:
            raise ValueError('instruction witness mismatch: '+hex(at))
    for at,expected in CONSTANTS.items():
        if struct.unpack('<d',read(pe,at,8))[0]!=expected:
            raise ValueError('floating-point constant mismatch: '+hex(at))
    return {'sha256':sha,'region_hashes':len(REGIONS),
            'near_call_targets':len(CALLS),'instruction_anchors':len(ANCHORS),
            'f64_constant_witnesses':len(CONSTANTS),
            'track_stride':228,'active_gate':'influence > 0',
            'cap_flag':'cap == 1.0',
            'history_valid':'current>=0 and previous in [current-1,current]',
            'original_binary_distributed':False}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',required=True,type=Path)
    args=parser.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as ex:parser.exit(1,'FAIL: '+str(ex)+'\n')
if __name__=='__main__':main()
