#!/usr/bin/env python3
"""Owner-private SHA-pinned audit for 2006 SiN GPV rating and interpolation.

No original DLL or game table rows are committed. The owner may optionally
write a private TSV for configuring the independent portable C++ model.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
NAME_TABLE=0x2245f04c
GPV_TABLE=0x22523478
GPV_ROW_BYTES=28
NAMES=(
 'DAMAGE_TAKEN_SCALE','DAMAGE_DONE_SCALE','AMMO_QUANTITY_SCALE',
 'NUM_FORCED_HEALTH_KITS','PERCENT_HEALTH_KITS_DROP','PERCENT_NPC_2ND_FIRE',
 'PERCENT_NPC_MAKE_DIFFERENT','PERCENT_NPC_CHANGE_SPAWN_NUM',
 'PERCENT_NPC_WEARS_HELMET','PERCENT_NPC_HELMET_SHOT_OFF',
 'PERCENT_NPC_TAKES_COVER','RANDOM_NPC_WAIT_BEFORE_SHOOT',
 'PERCENT_NPC_ACCURATE','PERCENT_NPC_MOBILE',
)
SIGS={
 0x222d3490:'d905f8223d22',     # load 1.0
 0x222d349b:'d8642404',         # (1-slider)
 0x222d34a2:'05c4000000',       # stride 196 (seven rows unrolled)
 0x222d34ac:'d88838ffffff',     # first endpoint multiply
 0x222d34b2:'d9442404',         # slider
 0x222d34b6:'d8883cffffff',     # second endpoint multiply
 0x222d34be:'d99834ffffff',     # store current row
 0x222d34a7:'3d08365222',       # end of 14 rows
 0x222d3379:'8da42400000000',   # name lookup prologue
 0x222d33c0:'83ff0e',           # name count 14
 0x222d3731:'a1cc275922',       # load active average owner
 0x222d373a:'8b430c',           # active reference count
 0x222d376f:'56',               # push GPV name
 0x222d3770:'e8fbfbffff',       # name index lookup
 0x222d3778:'83f8ff',           # unknown name check
 0x222d3793:'d9048d8c345222',   # +0x14 range bound
 0x222d37a1:'d94018',           # +0x18 bound
 0x222d37ac:'d900',             # +0x00 current
 0x222d37e0:'d8442410',         # accumulate contribution
 0x222d37f6:'db430c',           # divide by all reference count
 0x222d37fe:'d80d14633d22',    # scale by 100
}
class EvidenceError(ValueError):pass

def pe_read(blob,va,size):
    if len(blob)<0x100 or blob[:2]!=b'MZ':raise EvidenceError('missing PE header')
    pe=struct.unpack_from('<I',blob,0x3c)[0]
    if pe+24>len(blob) or blob[pe:pe+4]!=b'PE\0\0':
        raise EvidenceError('bad PE signature')
    machine,n,_,_,_,optsize,_=struct.unpack_from('<HHIIIHH',blob,pe+4)
    opt=pe+24
    if (machine!=0x14c or optsize<96 or opt+optsize>len(blob) or
        struct.unpack_from('<H',blob,opt)[0]!=0x10b):
        raise EvidenceError('expected PE32 i386')
    base=struct.unpack_from('<I',blob,opt+28)[0]
    rva=va-base
    if rva<0 or size<0 or n<1 or n>96:raise EvidenceError('PE address/section range')
    table=opt+optsize
    if table+40*n>len(blob):raise EvidenceError('PE section bounds')
    for i in range(n):
        p=table+40*i
        virtual,section,raw_size,raw_pos=struct.unpack_from('<IIII',blob,p+8)
        if section<=rva and rva+size<=section+min(virtual,raw_size):
            off=raw_pos+rva-section
            if off+size>len(blob):raise EvidenceError('PE data truncated')
            return blob[off:off+size]
    raise EvidenceError('PE VA not backed')

def verify(blob,*,sha=SHA256,names=NAMES,signatures=SIGS):
    if hashlib.sha256(blob).hexdigest()!=sha:
        raise EvidenceError('original DLL SHA mismatch')
    for va,expected in signatures.items():
        if pe_read(blob,va,len(expected)//2)!=bytes.fromhex(expected):
            raise EvidenceError('x86 instruction mismatch: '+hex(va))
    for va,value in ((0x223d6314,100.0),(0x223d22f8,1.0),(0x223d2590,0.0)):
        if pe_read(blob,va,4)!=struct.pack('<f',value):
            raise EvidenceError('rating multiplier/clamp constant mismatch at '+hex(va))
    records=[]
    for i,name in enumerate(names):
        pointer=struct.unpack('<I',pe_read(blob,NAME_TABLE+4*i,4))[0]
        if pe_read(blob,pointer,len(name)+1)!=name.encode('ascii')+b'\0':
            raise EvidenceError('GPV name mismatch at index '+str(i))
        values=struct.unpack('<7f',pe_read(blob,GPV_TABLE+i*GPV_ROW_BYTES,GPV_ROW_BYTES))
        if not all(math.isfinite(x) for x in values) or values[5]==values[6]:
            raise EvidenceError('invalid GPV float/range at index '+str(i))
        records.append({'name':name,'current':values[0],
             'low_target':values[1],'high_target':values[2],
             'rating_low':values[5],'rating_high':values[6]})
    return {
        'original_dll_sha256_verified':True,
        'verified_instruction_spans':len(signatures),
        'gpv_table_rows':len(records),
        'interpolation_va':'0x222d3490','rating_va':'0x222d3730',
        'rating_rule':'mean(clamp((current-rating_low)/(rating_high-rating_low),0,1))*100',
        'unknown_names_contribute_zero_but_count_in_denominator':True,
        'supports_descending_rating_ranges':True,
        'metric_values':records,
        'actual_stat_selection_and_update_schedule_verified':False,
        'original_code_executed':False,'quest_hmd_tested':False}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original_server_dll',type=Path)
    p.add_argument('--private-tsv',type=Path,
                   help='Keep generated per-row data in owner-private storage')
    args=p.parse_args()
    try:
        report=verify(args.original_server_dll.read_bytes())
        if args.private_tsv:
            columns=('current','low_target','high_target','rating_low','rating_high')
            header='name\t'+'\t'.join(columns)+'\n'
            lines=['\t'.join([row['name']]+[format(row[k],'.9g') for k in columns])
                   for row in report['metric_values']]
            args.private_tsv.write_text(header+'\n'.join(lines)+'\n',encoding='utf8')
        print(json.dumps(report,indent=2))
    except (OSError,ValueError,struct.error) as exc:
        p.exit(1,'GPV evidence failed: '+str(exc)+'\n')
if __name__=='__main__':main()
