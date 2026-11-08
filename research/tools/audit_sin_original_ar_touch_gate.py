#!/usr/bin/env python3
"""Read-only evidence for SiN Episodes 2006 CGrenadeAR original touch gate.

The original PRIVATE server.dll is supplied at runtime. This file contains
only independently authored decoding checks, VAs and metadata, NOT game bytes.
SHA256 pins the full original PE32 before any result is claimed.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA256 = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
TOUCH_VA = 0x223142f0
TOUCH_BYTES_SHA256 = "945fb670960ad71aba1a1e4f5e183122c35697fdef8f1021d9d99dae56197b91"

class AuditError(ValueError):
    pass

def need(test, detail):
    if not test:
        raise AuditError(detail)

class PE32View:
    def __init__(self, raw):
        need(len(raw) > 0x200 and raw[:2] == b"MZ", "missing DOS/PE header")
        pe = struct.unpack_from("<I", raw, 0x3c)[0]
        need(pe + 24 <= len(raw) and raw[pe:pe+4] == b"PE\x00\x00",
             "missing PE signature")
        machine, count = struct.unpack_from("<HH", raw, pe+4)
        optional_size = struct.unpack_from("<H", raw, pe+20)[0]
        opt = pe+24
        need(machine == 0x14c and 1 <= count <= 96 and
             opt+optional_size <= len(raw) and
             struct.unpack_from("<H",raw,opt)[0] == 0x10b,
             "expected i386 PE32 original game module")
        base=struct.unpack_from("<I",raw,opt+28)[0]
        need(base==0x22000000,"unexpected original server image base")
        self.raw=raw
        self.sections=[]
        for i in range(count):
            at=opt+optional_size+i*40
            need(at+40<=len(raw),"truncated PE sections")
            name=raw[at:at+8].split(b"\x00",1)[0].decode("ascii","replace")
            virt_size,rva,raw_size,offset=struct.unpack_from("<IIII",raw,at+8)
            need(offset+raw_size<=len(raw),"section exceeds original image")
            self.sections.append((name,base+rva,virt_size,raw_size,offset))
        need({".text",".rdata"} <= {item[0] for item in self.sections},
             "required original PE sections missing")

    def at(self,virtual_address,size):
        for name,va,virt_size,raw_size,offset in self.sections:
            if (va<=virtual_address and
                virtual_address+size<=va+min(virt_size,raw_size)):
                begin=offset+virtual_address-va
                return self.raw[begin:begin+size]
        raise AuditError("unmapped original PE RVA "+hex(virtual_address))

def rel32_target(view,site,call_prefix):
    data=view.at(site,len(call_prefix)+4)
    need(data[:len(call_prefix)]==call_prefix,
         "original near CALL opcode or register setup changed at "+hex(site))
    disp=struct.unpack_from("<i",data,len(call_prefix))[0]
    return site+len(call_prefix)+4+disp

def short_branch_target(view,site,opcode):
    raw=view.at(site,2)
    need(raw[0]==opcode,"original short branch opcode changed at "+hex(site))
    disp=struct.unpack("<b",raw[1:2])[0]
    return site+2+disp

def check_touch(view,strict_touch_hash=False):
    if strict_touch_hash:
        need(hashlib.sha256(view.at(TOUCH_VA,100)).hexdigest()==TOUCH_BYTES_SHA256,
             "original 100-byte touch handler fingerprint changed")
    need(view.at(0x223142f7,7)==bytes.fromhex("83 b9 1c 01 00 00 00"),
         "contact +0x11C guard changed")
    need(short_branch_target(view,0x223142fe,0x74)==0x22314350,
         "zero +0x11C no longer returns without detonation")
    need(view.at(0x22314300,7)==bytes.fromhex("f6 81 20 01 00 00 04"),
         "contact +0x120 bit4 filter changed")
    need(short_branch_target(view,0x22314307,0x75)==0x22314350,
         "contact mask bit4 no longer returns")
    need(view.at(0x22314310,6)==bytes.fromhex("ff 90 fc 00 00 00"),
         "other virtual method +0xFC changed")
    need(rel32_target(view,0x2231431c,b"\x8b\xcf\xe8")==0x222b2730,
         "original grenade related object helper call changed")
    need(view.at(0x22314323,2)==b"\x3b\xc6",
         "original related object comparison changed")
    need(short_branch_target(view,0x22314325,0x74)==0x2231433c,
         "matching related reference bypass changed")
    need(view.at(0x22314332,5)==bytes.fromhex("c6 44 24 10 01"),
         "original flag setter no longer receives 1")
    need(rel32_target(view,0x22314337,b"\xe8")==0x222b2b40,
         "original +0x795 setter helper call changed")
    need(view.at(0x2231433d,7)==bytes.fromhex("80 bf 95 07 00 00 00"),
         "original grenade flag +0x795 test changed")
    need(short_branch_target(view,0x22314344,0x74)==0x22314350,
         "original unarmed contact branch target changed")
    need(view.at(0x2231434a,6)==bytes.fromhex("ff 92 e8 03 00 00"),
         "original Detonate vtable offset +0x3E8 changed")
    need(view.at(0x22314350,4)==bytes.fromhex("5f c2 04 00"),
         "original touch stdcall return changed")
    need(view.at(0x22314c4d,7)==bytes.fromhex("80 be 95 07 00 00 00"),
         "original spawn flag condition changed")
    need(view.at(0x22314c73,7)==bytes.fromhex("c6 86 95 07 00 00 00"),
         "original spawn no longer clears byte +0x795")
    return {
        "source":"owner-provided SiN Episodes Emergence 2006 server.dll",
        "original_server_sha256":SERVER_SHA256,
        "touch_function_va":hex(TOUCH_VA),
        "touch_handler_100_byte_sha256":TOUCH_BYTES_SHA256,
        "contact_field_guard_offset":hex(0x11c),
        "contact_flag_field_offset":hex(0x120),
        "contact_reject_flag_mask":hex(0x4),
        "contact_related_virtual_method_offset":hex(0xfc),
        "related_helper_call_target":hex(0x222b2730),
        "conditional_flag_setter_va":hex(0x222b2b40),
        "grenade_original_flag_offset":hex(0x795),
        "original_spawn_clears_flag_va":hex(0x22314c73),
        "detonate_virtual_slot":250,
        "detonate_virtual_byte_offset":hex(0x3e8),
        "first_trigger_predicate_types_fully_identified":False,
        "sourcevr_entity_integration_proven":False,
    }

def audit(path):
    raw=Path(path).read_bytes()
    need(hashlib.sha256(raw).hexdigest()==SERVER_SHA256,
         "original 2006 server.dll SHA256 mismatch")
    return check_touch(PE32View(raw),strict_touch_hash=True)

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",required=True,type=Path)
    args=p.parse_args()
    try:
        print(json.dumps(audit(args.server),indent=2))
    except (AuditError,OSError,struct.error) as exc:
        p.exit(2,f"Original SiN 2006 grenade touch audit FAILED: {exc}\n")
