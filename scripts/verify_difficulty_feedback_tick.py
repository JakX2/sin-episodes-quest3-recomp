#!/usr/bin/env python3
"""SHA-pinned original 2006 SiN difficulty tick/caller instruction audit.

Original server.dll remains private. This verifies timed branch/ordering and
does NOT reveal the actual performance weighting of opaque host callbacks.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SHA256 = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
TICK_VA = 0x222ba420
COLLECT_VA = 0x222b9e80
CALLER_VA = 0x222bc0e0
EXPECTED_FOLLOW_UP_TARGETS = (
    0x222bae60, 0x222bbbd0, 0x222bb0b0,
    0x222bf510, 0x222bbac0, 0x222ba4e0,
)
FOLLOW_UP_CALLS = (
    0x222bc0f3, 0x222bc108, 0x222bc10f,
    0x222bc117, 0x222bc11e, 0x222bc125,
)
SIGNATURES = {
    0x222ba423: 'd94660',              # next due at object+0x60
    0x222ba426: 'd85c2408',            # game_time argument
    0x222ba42c: 'f6c441',              # x87 status bits
    0x222ba42f: '7a39',                # skip when not due
    0x222ba431: '8b463c',              # half-interval count at +0x3c
    0x222ba439: '7e11',                # count <= 0 uses full interval
    0x222ba43d: '83c0ff',              # counter decrement
    0x222ba443: '89463c',              # store updated counter
    0x222ba446: 'd80d78973d22',       # multiply by original 0.5f
    0x222ba44c: 'd8442408',            # add current time
    0x222ba452: 'd95e60',              # store next due
    0x222ba45a: '807e0500',            # test pending +0x05
    0x222ba460: 'c6460500',            # clear pending flag
    0x222ba464: 'b001',                # return true
    0x222ba46a: '32c0',                # return false
    0x222b9f22: '83c601',              # category index increment
    0x222b9f28: '83fe06',              # six category iterations
    0x222bc0ed: '84c0',                # test tick result
    0x222bc0ef: '7439',                # skip all followups if false
    0x222bc0fc: '898694000000',        # first counter reset
    0x222bc102: '898698000000',        # second counter reset
}
class EvidenceError(ValueError): pass

def sections(blob):
    if len(blob)<0x100 or blob[:2]!=b'MZ':
        raise EvidenceError('invalid PE DOS header')
    at=struct.unpack_from('<I',blob,0x3c)[0]
    if at+24>len(blob) or blob[at:at+4]!=b'PE\0\0':
        raise EvidenceError('invalid PE signature')
    machine,n,_,_,_,opt_size,_=struct.unpack_from('<HHIIIHH',blob,at+4)
    opt=at+24
    if (machine!=0x14c or not 1<=n<=96 or opt_size<96 or
        opt+opt_size>len(blob) or struct.unpack_from('<H',blob,opt)[0]!=0x10b):
        raise EvidenceError('expected i386 PE32')
    base=struct.unpack_from('<I',blob,opt+28)[0]
    table=opt+opt_size
    if table+40*n>len(blob):raise EvidenceError('truncated section table')
    result=[]
    for i in range(n):
        row=table+i*40
        name=blob[row:row+8].split(b'\0')[0].decode('ascii','replace')
        vsize,rva,size,off=struct.unpack_from('<IIII',blob,row+8)
        if off+size>len(blob):raise EvidenceError('truncated section')
        result.append((name,base+rva,off,min(vsize,size)))
    return result

def read(blob,va,n):
    for _,start,off,size in sections(blob):
        if n>=0 and start<=va and va+n<=start+size:
            return blob[off+va-start:off+va-start+n]
    raise EvidenceError('VA not backed: '+hex(va))

def direct_target(blob,va):
    code=read(blob,va,5)
    if code[0]!=0xe8:raise EvidenceError('not direct CALL '+hex(va))
    return va+5+struct.unpack_from('<i',code,1)[0]

def call_sites(blob,target):
    result=[]
    for name,start,off,size in sections(blob):
        if name!='.text':continue
        for i in range(size-4):
            if blob[off+i]!=0xe8:continue
            if start+i+5+struct.unpack_from('<i',blob,off+i+1)[0]==target:
                result.append(start+i)
    return sorted(result)

def verify(blob,*,sha=SHA256,signatures=SIGNATURES):
    if hashlib.sha256(blob).hexdigest()!=sha:
        raise EvidenceError('original server.dll SHA mismatch')
    for va,code in signatures.items():
        if read(blob,va,len(code)//2)!=bytes.fromhex(code):
            raise EvidenceError('instruction mismatch at '+hex(va))
    if read(blob,0x223d9778,4)!=struct.pack('<f',0.5):
        raise EvidenceError('half interval constant changed')
    if direct_target(blob,0x222ba455)!=COLLECT_VA:
        raise EvidenceError('category collector target changed')
    if direct_target(blob,0x222bc0e8)!=TICK_VA:
        raise EvidenceError('timed gate caller changed')
    if call_sites(blob,TICK_VA)!=[0x222bc0e8]:
        raise EvidenceError('timed gate direct caller inventory changed')
    if call_sites(blob,COLLECT_VA)!=[0x222ba455]:
        raise EvidenceError('collector direct caller inventory changed')
    for site,target in zip(FOLLOW_UP_CALLS,EXPECTED_FOLLOW_UP_TARGETS):
        if direct_target(blob,site)!=target:
            raise EvidenceError('downstream call target changed at '+hex(site))
    return {
        'verified_original_hash':True,
        'instruction_spans':len(signatures),
        'gate_va':hex(TICK_VA),
        'gate_caller_va':hex(CALLER_VA),
        'collector_va':hex(COLLECT_VA),
        'collector_category_slots':6,
        'follow_up_stage_count':len(FOLLOW_UP_CALLS),
        'next_due_field':'+0x60',
        'normal_interval_field':'+0x64',
        'accelerated_tick_counter_field':'+0x3c',
        'one_shot_flag_field':'+0x05',
        'temporary_interval_scale':0.5,
        'wrapper_reset_counter_offsets':['+0x94','+0x98'],
        'player_performance_weights_recovered':False,
        'sourcevr_game_module_loaded':False,
        'quest_headset_tested':False,
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original_server_dll',type=Path)
    a=p.parse_args()
    try:print(json.dumps(verify(a.original_server_dll.read_bytes()),indent=2,sort_keys=True))
    except (OSError,ValueError,struct.error) as exc:
        p.exit(1,'difficulty tick evidence failed: '+str(exc)+'\n')
if __name__=='__main__':main()
