#!/usr/bin/env python3
"""Read-only, SHA-pinned audit of the 2006 SiN feedback flag publisher.

Original server.dll is owner-held and never committed. This checks exact
machine instruction spans and *does not* recover surrounding player AI/stats.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SHA256="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
INSTRUCTIONS={
  0x222bc3e3:"807e0400",       # cmp byte [esi+4],0: active flag
  0x222bc3e7:"0f84a6000000",   # skip timer and tick when not active
  0x222bc463:"d94674",         # load report deadline [esi+0x74]
  0x222bc466:"d85c2424",       # compare time arg
  0x222bc46a:"dfe0",           # x87 status -> AX
  0x222bc46c:"f6c441",         # status comparator mask
  0x222bc46f:"7a0e",           # if deadline in future, skip flag write
  0x222bc471:"d9442424",       # load now
  0x222bc475:"c6460501",       # publish this+5 = 1
  0x222bc479:"d84668",         # add original interval this+0x68
  0x222bc47c:"d95e74",         # store new deadline this+0x74
  0x222bc47f:"8b15a8235922",   # read registered global setting pointer
  0x222bc485:"837a2c00",       # compare setting object's +0x2c to 0
  0x222bc489:"7408",           # skip downstream gate if disabled
  0x222bc48b:"55",             # push original call arg
  0x222bc48e:"e84dfcffff",     # CALL 0x222BC0E0, Phase95 wrapper
  0x222ba460:"c6460500",       # Phase95 downstream gate clears flag
}
class EvidenceError(ValueError):pass

def read_pe_va(data,va,n):
    if len(data)<0x100 or data[:2]!=b"MZ":
        raise EvidenceError("invalid PE DOS header")
    pe=struct.unpack_from("<I",data,0x3c)[0]
    if pe+24>len(data) or data[pe:pe+4]!=b"PE\0\0":
        raise EvidenceError("invalid PE signature")
    machine,count,_,_,_,opt_size,_=struct.unpack_from("<HHIIIHH",data,pe+4)
    opt=pe+24
    if machine!=0x14c or not 1<=count<=96 or opt_size<96 or opt+opt_size>len(data):
        raise EvidenceError("expected i386 PE32")
    if struct.unpack_from("<H",data,opt)[0]!=0x10b:
        raise EvidenceError("expected PE32 optional header")
    base=struct.unpack_from("<I",data,opt+28)[0]
    rva=va-base
    if n<0 or rva<0:raise EvidenceError("invalid RVA")
    table=opt+opt_size
    if table+40*count>len(data):raise EvidenceError("truncated PE section table")
    for i in range(count):
        at=table+40*i
        virtual,sec,raw_size,off=struct.unpack_from("<IIII",data,at+8)
        if sec<=rva and rva+n<=sec+min(virtual,raw_size):
            pos=off+rva-sec
            if pos+n>len(data):raise EvidenceError("truncated PE data")
            return data[pos:pos+n]
    raise EvidenceError("unmapped original virtual address")

def verify(original,*,sha=SHA256,signatures=INSTRUCTIONS):
    if hashlib.sha256(original).hexdigest()!=sha:
        raise EvidenceError("original server.dll SHA256 mismatch")
    for va,code in signatures.items():
        expected=bytes.fromhex(code)
        if read_pe_va(original,va,len(expected))!=expected:
            raise EvidenceError("publisher instruction mismatch at "+hex(va))
    rel=struct.unpack("<i",read_pe_va(original,0x222bc48f,4))[0]
    if 0x222bc493+rel!=0x222bc0e0:
        raise EvidenceError("original feedback wrapper call target mismatch")
    return {
        "original_sha256_verified":True,
        "publication_writer_va":"0x222bc475",
        "publication_deadline_offset":"+0x74",
        "publication_interval_offset":"+0x68",
        "active_flag_offset":"+0x04",
        "pending_flag_offset":"+0x05",
        "global_setting_pointer_va":"0x225923a8",
        "setting_enable_field_offset":"+0x2c",
        "downstream_wrapper_va":"0x222bc0e0",
        "flag_consumer_va":"0x222ba460",
        "verified_instruction_spans":len(signatures),
        "complete_stats_feedback_logic_recovered":False,
        "original_code_executed":False,
        "quest_headset_tested":False
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("original_server_dll",type=Path)
    args=p.parse_args()
    try:print(json.dumps(verify(args.original_server_dll.read_bytes()),indent=2,sort_keys=True))
    except (OSError,ValueError,struct.error) as e:
        p.exit(1,"Phase97 publisher verification failed: "+str(e)+"\n")
if __name__=="__main__":main()
