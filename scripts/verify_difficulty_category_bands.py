#!/usr/bin/env python3
"""Read-only original SiN 2006 difficulty category-band and log-helper audit.
Pinned by whole-file SHA256; does not publish/execute proprietary DLL content.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
TABLE=0x22521de0
LOWER=0x223d9cb0
UPPER=0x223d9cc0
LOG_FILENAME=0x2245cf7c
# Signature list: (original VA, short instruction bytes). No full code dumps.
SIGNATURES=dict(
    (int(va,16),code) for va,code in (
        row.split(':') for row in """
222c0260:a100245922
222c0265:83782c00
222c0283:687ccf4522
222b9f18:b900215922
222b9f1d:e83e630000
222c0740:8b442404
222c0744:c1e004
222c0747:05e01d5222
222c074c:837c240802
222c0753:83c008
222c0770:8b442404
222c0774:c1e004
222c0777:05e01d5222
222c077c:8b4804
222c077f:8b500c
222c0782:8908
222c0784:895008
222c07a0:8b442404
222c07ac:837c240802
222c07b3:d900
222c07b6:d94008
222c07c0:8b4c2404
222c07d7:d944240c
222c07db:d801
222c07dd:d815c09c3d22
222c07ec:c70100000040
222c07f3:d815b09c3d22
222c0802:c701cdcc4c3d
222c0809:d919
222c0810:d944240c
222c0846:8b442404
222c0852:837c240802
222c0859:d918
222c085c:d95808
222c0860:a1e41d5222
222c0871:a3e01d5222
222baef8:e8c3580000
222b926c:e89f750000
222b9279:e892750000
222bba88:e8d34d0000
""".strip().splitlines()
    )
)
class EvidenceError(ValueError): pass

def read_pe_va(original,va,n):
    if len(original)<128 or original[:2]!=b'MZ':
        raise EvidenceError('invalid DOS header')
    off=struct.unpack_from('<I',original,0x3c)[0]
    if off+24>len(original) or original[off:off+4]!=b'PE\0\0':
        raise EvidenceError('invalid PE header')
    machine,count,_,_,_,opt_size,_=struct.unpack_from('<HHIIIHH',original,off+4)
    opt=off+24
    if (machine!=0x14c or not 1<=count<=96 or opt_size<96 or
        opt+opt_size>len(original) or struct.unpack_from('<H',original,opt)[0]!=0x10b):
        raise EvidenceError('expected PE32 i386')
    base=struct.unpack_from('<I',original,opt+28)[0]
    relative=va-base
    table=opt+opt_size
    if table+40*count>len(original) or relative<0 or n<0:
        raise EvidenceError('invalid PE table/VA')
    for i in range(count):
        row=table+40*i
        vsize,rva,raw_size,raw=struct.unpack_from('<IIII',original,row+8)
        if rva<=relative and relative+n<=rva+min(vsize,raw_size):
            at=raw+relative-rva
            if at+n>len(original):raise EvidenceError('truncated section')
            return original[at:at+n]
    raise EvidenceError('VA not backed')

def verify(original,*,sha=SHA256,signatures=SIGNATURES):
    if hashlib.sha256(original).hexdigest()!=sha:
        raise EvidenceError('original server.dll SHA mismatch')
    for va,expected in signatures.items():
        if read_pe_va(original,va,len(expected)//2)!=bytes.fromhex(expected):
            raise EvidenceError('original instruction mismatch: '+hex(va))
    minimum=struct.unpack('<f',read_pe_va(original,LOWER,4))[0]
    maximum=struct.unpack('<f',read_pe_va(original,UPPER,4))[0]
    if minimum!=struct.unpack('<f',bytes.fromhex('cdcc4c3d'))[0] or maximum!=2.0:
        raise EvidenceError('original clamp limits mismatch')
    if read_pe_va(original,LOG_FILENAME,12)!=b'dd_info.log\0':
        raise EvidenceError('original logging filename mismatch')
    for index in range(6):
        row=struct.unpack('<4f',read_pe_va(original,TABLE+index*16,16))
        if row!=(1.0,1.0,1.0,1.0):
            raise EvidenceError('original initial category factors mismatch at '+str(index))
    return {
        'original_sha256_verified':True,
        'verified_instruction_signatures':len(signatures),
        'category_count':6,'bands_per_category':2,
        'category_row_stride_bytes':16,'category_table_va':hex(TABLE),
        'four_float_layout':['low_current','low_saved','high_current','high_saved'],
        'current_offsets':['+0x0','+0x8'],'saved_offsets':['+0x4','+0xc'],
        'band_selector_split':2,'factor_minimum':minimum,'factor_maximum':maximum,
        'initial_factors_verified':True,
        'additive_modifier_va':'0x222c07c0',
        'absolute_modifier_va':'0x222c0810',
        'reset_saved_baselines_va':'0x222c0860',
        'category_collector_helper_is_diagnostic_logging':True,
        'diagnostic_logger_va':'0x222c0260','diagnostic_filename':'dd_info.log',
        'original_feedback_weight_sign_logic_recovered':False,
        'original_code_executed':False,'quest_headset_tested':False
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original_server_dll',type=Path)
    a=p.parse_args()
    try:print(json.dumps(verify(a.original_server_dll.read_bytes()),indent=2,sort_keys=True))
    except (OSError,ValueError,struct.error) as exc:
        p.exit(1,'Phase98 audit failed: '+str(exc)+'\n')
if __name__=='__main__':main()
