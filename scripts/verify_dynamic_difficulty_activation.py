#!/usr/bin/env python3
"""Pinned, read-only 2006 SiN server.dll audit of GPV interpolation callers.
No original DLL bytes are published or executed. The surrounding feedback
controller's update schedule remains unverified.
"""
import argparse, hashlib, json, struct
from pathlib import Path

SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
TARGET=0x222d3490
CALLS=(0x222b9194,0x222ba3ba)
RESTORE_CALLER=0x222bbfda
SIGNATURES={
    0x222b915a:'83be6402000000',       # check this+0x264
    0x222b9161:'7543',                 # branch if initialized
    0x222b9189:'8b4608',               # cached slider this+0x8
    0x222b918c:'50',                   # push cached slider
    0x222b918d:'c7460c0000003f',       # +0x0c = 0.5f
    0x222b919c:'c7866402000001000000',# initialized=true
    0x222ba363:'c7866402000001000000',# direct initialization flag
    0x222ba399:'8b0d68215922',        # global ConVar pointer
    0x222ba39f:'8b5128',              # +0x28 ConVar value
    0x222ba3a2:'895608',              # this+0x8 cached setting
    0x222ba3b0:'8b0d68215922',
    0x222ba3b6:'8b5128',
    0x222ba3b9:'52',                  # push value
    0x222ba3c8:'e8c3670000',          # another unmodeled subsystem call
    0x222bbfd8:'8bce',                # this for guarded restore
}
class ActivationEvidenceError(ValueError):pass
def pe_sections(raw):
    if len(raw)<0x100 or raw[:2]!=b'MZ':
        raise ActivationEvidenceError('invalid PE DOS header')
    at=struct.unpack_from('<I',raw,0x3c)[0]
    if at+24>len(raw) or raw[at:at+4]!=b'PE\0\0':
        raise ActivationEvidenceError('invalid PE signature')
    machine,n,_,_,_,optsize,_=struct.unpack_from('<HHIIIHH',raw,at+4)
    opt=at+24
    if machine!=0x14c or n<1 or n>96 or optsize<96 or opt+optsize>len(raw) or struct.unpack_from('<H',raw,opt)[0]!=0x10b:
        raise ActivationEvidenceError('expected PE32 i386')
    base=struct.unpack_from('<I',raw,opt+28)[0]
    table=opt+optsize
    if table+40*n>len(raw):raise ActivationEvidenceError('bad section table')
    sections=[]
    for i in range(n):
        p=table+i*40
        name=raw[p:p+8].split(b'\0')[0].decode('ascii','replace')
        virtual,rva,size,offset=struct.unpack_from('<IIII',raw,p+8)
        if offset+size>len(raw):raise ActivationEvidenceError('truncated PE section')
        sections.append((name,base+rva,offset,min(virtual,size)))
    return sections
def pe_read(raw,va,n):
    for _,base,offset,span in pe_sections(raw):
        if n>=0 and base<=va and va+n<=base+span:
            return raw[offset+va-base:offset+va-base+n]
    raise ActivationEvidenceError('VA not backed: '+hex(va))
def call_target(raw,va):
    code=pe_read(raw,va,5)
    if code[0]!=0xe8:
        raise ActivationEvidenceError('expected rel32 CALL at '+hex(va))
    return va+5+struct.unpack_from('<i',code,1)[0]
def direct_call_sites(raw,target):
    hits=[]
    for name,base,offset,span in pe_sections(raw):
        if name!='.text':continue
        for i in range(span-4):
            if raw[offset+i]!=0xe8:continue
            va=base+i
            if va+5+struct.unpack_from('<i',raw,offset+i+1)[0]==target:hits.append(va)
    return sorted(hits)
def verify(raw,*,sha=SHA256,signatures=SIGNATURES,calls=CALLS):
    if hashlib.sha256(raw).hexdigest()!=sha:
        raise ActivationEvidenceError('original DLL SHA-256 mismatch')
    for va,expected in signatures.items():
        if pe_read(raw,va,len(expected)//2)!=bytes.fromhex(expected):
            raise ActivationEvidenceError('instruction signature mismatch at '+hex(va))
    for va in calls:
        if call_target(raw,va)!=TARGET:
            raise ActivationEvidenceError('interpolation call target changed at '+hex(va))
    if call_target(raw,RESTORE_CALLER)!=0x222b9130:
        raise ActivationEvidenceError('guarded restore call target changed')
    if direct_call_sites(raw,TARGET)!=list(calls):
        raise ActivationEvidenceError('original interpolation caller inventory changed')
    return {
        'full_dll_hash_verified':True,
        'interpolation_target_va':hex(TARGET),
        'exact_direct_call_sites':[hex(c) for c in calls],
        'verified_instruction_spans':len(signatures),
        'guarded_restore_va':'0x222b9130',
        'guarded_restore_caller_va':hex(RESTORE_CALLER),
        'guard_field_offset':'0x264',
        'cached_setting_offset':'0x8',
        'guarded_restore_half_marker_offset':'0xc',
        'full_initialize_va':'0x222ba360',
        'full_initialize_convar_pointer_va':'0x22592168',
        'convar_value_offset':'0x28',
        'original_full_update_schedule_recovered':False,
        'challenge_feedback_formula_recovered':False,
        'sourcevr_game_module_loaded':False,
        'quest_headset_tested':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original_server_dll',type=Path)
    a=p.parse_args()
    try:print(json.dumps(verify(a.original_server_dll.read_bytes()),indent=2,sort_keys=True))
    except (OSError,ValueError,struct.error) as err:
        p.exit(1,'Phase94 original caller evidence failed: '+str(err)+'\n')
if __name__=='__main__':main()
