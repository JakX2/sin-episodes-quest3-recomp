#!/usr/bin/env python3
"""Phase608: exact original SiN 2006 Grunt Follow eligibility continuation.

Reads SHA-verified retail DLLs only; no original executable bytes are exported.
The stable-handle semantic model is intentionally bounded: native re-reads the
handle and may dereference null if it changes between its two resolutions.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER, CLIENT, PE32, Refusal, require
)

FOLLOW_VTABLE=0x223F6B7C
FOLLOW_OUTER=(0x220D15D0,31,
    'c60968717e032c273c8f7a7c5d32edb08f3f8d8d44aa3781d3f68d11579fd542')
FOLLOW_INNER=(0x220D2370,115,
    'ceffc4ba3ad0438c39110002eb8734ad6010061e27733a72bae8627473113b4d')
NEXT_METHOD=0x220D23F0
SERIAL_TABLE_GLOBAL=0x224F5620
WITNESSES={
  0x220D15D8:'ff9050040000',  # owner virtual +0x450 gate
  0x220D15E9:'ffa2e4000000',  # tail dispatch behavior virtual +0xE4
  0x220D2370:'8b4114',        # behavior+0x14 encoded handle, first read
  0x220D2373:'83f8ff',        # invalid -1 sentinel
  0x220D2379:'8b3520564f22',  # original serial-checked global entity table
  0x220D2381:'81e2ff0f0000', # low 12-bit entity table index
  0x220D2387:'c1e204',       # 16-byte table entry
  0x220D238E:'c1e80c',       # high bits = serial
  0x220D2391:'394204',       # serial equality
  0x220D2396:'8b12',         # slot entity pointer
  0x220D2398:'85d2',         # non-null first lookup
  0x220D239C:'32c0',         # early false
  0x220D23A0:'8b4114',       # separate second handle read
  0x220D23AA:'81e1ff0f0000',# second low-index mask
  0x220D23B7:'c1e80c',       # second high serial
  0x220D23BA:'394104',       # second serial equality
  0x220D23BF:'8b09',         # second entity pointer
  0x220D23C1:'8b81bc000000',# entity+0xBC field
  0x220D23C7:'c1e80f',       # test original bit 15
  0x220D23CA:'f7d0',         # invert
  0x220D23CC:'83e001',       # return 1 iff bit 15 clear
  0x220D23D1:'33c9',         # second lookup failure sets ecx=0
  0x220D23D3:'8b81bc000000',# then dereferences [ecx+0xBC], may fault
}

def stable_handle_eligible(owner_gate, handle, matching_serial, entity_present, flags):
    """Portable behavioral *subset* under unchanging valid handle.
    Original x86 can crash if the second lookup fails after first passes.
    The safe model refuses stale/missing entities rather than reproducing that.
    """
    return (bool(owner_gate) and handle!=0xffffffff and
            bool(matching_serial) and bool(entity_present) and
            (flags & 0x8000)==0)

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            'Unexpected original 2006 server.dll SHA')
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            'Unexpected original 2006 client.dll SHA')
    pe=PE32(server)
    outer,nouter,outer_sha=FOLLOW_OUTER
    inner,ninner,inner_sha=FOLLOW_INNER
    require(pe.section(FOLLOW_VTABLE)=='.rdata',
            'Original Follow RTTI vtable outside .rdata')
    require(pe.u32(FOLLOW_VTABLE+0x18)==outer,
            'Original Follow eligibility slot +0x18 differs')
    require(pe.u32(FOLLOW_VTABLE+0xE4)==inner,
            'Original Follow subpredicate vtable slot +0xE4 differs')
    require(hashlib.sha256(pe.read(outer,nouter)).hexdigest()==outer_sha,
            'Original outer Follow eligibility method changed')
    require(pe.read(outer+nouter,1)==b'\xcc',
            'Original outer Follow boundary is not one INT3')
    require(hashlib.sha256(pe.read(inner,ninner)).hexdigest()==inner_sha,
            'Complete native Follow +0xE4 subpredicate hash differs')
    require(pe.read(inner+ninner,13)==b'\xcc'*13,
            'Follow subpredicate requires 13 native INT3 pad bytes')
    require(pe.read(NEXT_METHOD,3)==bytes.fromhex('8b5104'),
            'Distinct next native method boundary differs')
    for va,hexcode in WITNESSES.items():
        expect=bytes.fromhex(hexcode)
        require(pe.read(va,len(expect))==expect,
                'Original control/handle/flag witness altered '+hex(va))
    return {
      'status':'PHASE608_ORIGINAL_FOLLOW_SUBPREDICATE_AUTHENTICATED',
      'original_server_sha256':SERVER,
      'original_client_sha256':CLIENT,
      'phase607_follow_outer_reauthenticated':'0x%08X'%outer,
      'new_original_complete_methods':1,
      'new_original_complete_method_bytes':ninner,
      'new_whole_original_method_start':'0x%08X'%inner,
      'new_whole_original_method_sha256':inner_sha,
      'native_post_method_int3_padding_verified':13,
      'next_distinct_original_method':'0x%08X'%NEXT_METHOD,
      'original_follow_rtti_vtable':'0x%08X'%FOLLOW_VTABLE,
      'original_follow_eligibility_slot':'0x18',
      'original_follow_eligibility_continuation_slot':'0xE4',
      'raw_handle_member_offset':'0x14',
      'entity_index_mask':'0xFFF',
      'entity_handle_serial_shift':12,
      'original_entity_table_global':'0x%08X'%SERIAL_TABLE_GLOBAL,
      'entity_slot_size_bytes':16,
      'original_entity_flags_offset':'0xBC',
      'flags_bit_that_disqualifies':'0x8000',
      'stable_handle_requires_owner_gate_and_valid_handle_and_bit15_clear':True,
      'native_repeats_serial_lookup_twice':True,
      'native_second_lookup_failure_may_dereference_null':True,
      'portable_reference_must_fail_closed_on_missing_entity':True,
      'sparse_source_opcode_witnesses':len(WITNESSES),
      'real_engine_handle_lifetime_contract_proven':False,
      'original_symbolic_flag_name_proven':False,
      'original_gameplay_port_boot_proven':False,
      'proprietary_machine_code_exported':False,
    }

def self_test():
    import itertools
    cases=0
    for gate,handle_ok,serial_ok,present,bit_set in itertools.product(
            (False,True),repeat=5):
        handle=0x00012345 if handle_ok else 0xffffffff
        flags=0x8000 if bit_set else 0
        result=stable_handle_eligible(gate,handle,serial_ok,present,flags)
        expected=gate and handle_ok and serial_ok and present and not bit_set
        assert result==expected
        cases+=1
    assert cases==32
    assert stable_handle_eligible(True,0x12345678,True,True,0x7FFF)
    assert not stable_handle_eligible(True,0x12345678,True,True,0x8000)
    assert not stable_handle_eligible(True,0x12345678,False,True,0)
    assert not stable_handle_eligible(True,0x12345678,True,False,0)
    assert len(WITNESSES)>=20
    try:audit(b'corrupt',b'corrupt')
    except Refusal:pass
    else:raise AssertionError('Accepted wrong original retail binary pair')
    print('PHASE608_ORIGINAL_FREE_FOLLOW_HANDLE_BIT15_SYNTHETIC_PASS (32 cases)')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.self_test:
        self_test()
        if not(a.bin_zip or a.server):return
    if a.bin_zip is None and not(a.server and a.client):
        p.error('Provide --bin-zip or paired --server --client or --self-test')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (Refusal,OSError,KeyError,struct.error,zipfile.BadZipFile) as ex:
        p.exit(2,'PHASE608 ORIGINAL SOURCE REFUSED: %s\n'%ex)
    output=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Verified complete original Follow +0xE4:',a.out)
    else:print(output)

if __name__=='__main__':main()
