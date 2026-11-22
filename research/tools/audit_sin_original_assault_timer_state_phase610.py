#!/usr/bin/env python3
"""Phase610: check original 2006 Assault eligibility x87/timer/marker dataflow.

Read-only SHA-pinned PE32 audit; exports only hashes and semantic metadata,
not the original private game executable or its complete disassembly.
"""
import argparse
import hashlib
import json
import math
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER, CLIENT, PE32, Refusal, require, check_edge
)
ENTRY=0x220CEAF0
SIZE=255
SHA="0c5b688341149ae7ed28a49f9f930c87116810c67f1107a444e48742ba570466"
ASSAULT_VTABLE=0x223F5544
ZERO_FLOAT=0x223D2590
FOUR_FLOAT=0x223DDCF4
DIRECT={
    0x220CEB07:0x220397C0, # owner condition check 0x3B
    0x220CEB3A:0x220CE240, # first missing-target predicate
    0x220CEB4A:0x220CE350, # second missing-target predicate
    0x220CEB73:0x220CE210, # first target resolver
    0x220CEB7E:0x220CE2F0, # re-resolve first handle
    0x220CEBCE:0x22039760, # signal condition 0x18
}
# All observed in exact original 2006 PE32. Field and FPU witnesses
# are intentionally short so no bulk original code is exported.
WITNESSES={
    0x220CEAF8:"ff9050040000",0x220CEB05:"6a3b",
    0x220CEB10:"d9462c",0x220CEB13:"8b0d90215722",
    0x220CEB19:"d8590c",0x220CEB1E:"f6c441",
    0x220CEB21:"7504",0x220CEB2A:"395e1c",
    0x220CEB35:"8d7e14",0x220CEB47:"8d4e18",
    0x220CEB5C:"ff9254010000",0x220CEB66:"a190215722",
    0x220CEB6E:"894e28",0x220CEB83:"3898b9030000",
    0x220CEB8B:"d90590253d22",0x220CEB91:"d94628",
    0x220CEB94:"dae9",0x220CEB98:"f6c444",
    0x220CEB9B:"7b27",0x220CEBA3:"d9420c",
    0x220CEBA6:"d86628",0x220CEBA9:"d81df4dc3d22",
    0x220CEBB1:"f6c405",0x220CEBB4:"7a0e",
    0x220CEBB6:"c6462601",0x220CEBC4:"385e26",
    0x220CEBCC:"6a18",0x220CEBD3:"885e26",
    0x220CEBD6:"385e26",0x220CEBDB:"0f94c0",
    0x220CEBE1:"895e1c",0x220CEBE4:"885e24",
    0x220CEBE7:"885e25",
}

def model_ordered_finite(deadline,now,time28,flag26,primary_byte_nonzero,
                         owner_154_result_nonnull):
    """Only the late state machine for stable targets after all early gates.
    Caller supplies established present handles and nonzero +0x1C state.
    Returns (eligible,new_time28,new_flag26,signal_condition18).
    """
    if not all(map(math.isfinite,(deadline,now,time28))):
        return False,time28,flag26,False
    if deadline>now:return False,time28,flag26,False
    if owner_154_result_nonnull:time28=now
    if not primary_byte_nonzero:
        return flag26==0,time28,flag26,False
    if time28==0.0 and (now-time28)>=4.0:
        return False,time28,1,False
    return True,time28,0,flag26!=0

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            "Unsupported original server.dll SHA256")
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            "Unsupported original client.dll SHA256")
    pe=PE32(server)
    require(pe.u32(ASSAULT_VTABLE+0x18)==ENTRY,
            "Original Assault +0x18 vtable pointer differs")
    require(hashlib.sha256(pe.read(ENTRY,SIZE)).hexdigest()==SHA,
            "Original full Assault eligibility body changed")
    require(pe.read(ENTRY+SIZE,1)==b'\xcc',
            "Original Assault end is a single INT3")
    require(pe.read(ENTRY+SIZE+1,4)==bytes.fromhex("c6412400"),
            "Separate following original function opener differs")
    require(pe.section(ZERO_FLOAT)=='.rdata' and pe.section(FOUR_FLOAT)=='.rdata',
            "Original x87 constants are not located in .rdata")
    require(struct.unpack("<f",pe.read(ZERO_FLOAT,4))[0]==0.0,
            "Original comparison float !=0.0")
    require(struct.unpack("<f",pe.read(FOUR_FLOAT,4))[0]==4.0,
            "Original comparison float !=4.0")
    for site,target in DIRECT.items():
        check_edge(pe.read,site,target,0xE8)
    for site,hexbytes in WITNESSES.items():
        raw=bytes.fromhex(hexbytes)
        require(pe.read(site,len(raw))==raw,
                "Original x87/Assault state witness changed "+hex(site))
    return {
       "status":"PHASE610_ORIGINAL_ASSAULT_FINITE_TIME_STATE_AUTHENTICATED",
       "original_server_sha256":SERVER,
       "original_client_sha256":CLIENT,
       "existing_original_method":"0x%08X"%ENTRY,
       "existing_original_whole_method_bytes":SIZE,
       "existing_original_whole_sha256":SHA,
       "new_distinct_native_original_methods":0,
       "new_original_method_bytes":0,
       "single_INT3_function_boundary_verified":True,
       "float_constant_zero_original_va":"0x%08X"%ZERO_FLOAT,
       "float_constant_duration4_original_va":"0x%08X"%FOUR_FLOAT,
       "native_E8_edges_verified":len(DIRECT),
       "native_short_witnesses":len(WITNESSES),
       "original_deadline_plus_0x2c_must_be_at_or_before_now_for_finite":True,
       "state_plus_0x1c_nonzero_required":True,
       "two_target_missing_short_circuits_and_clears_plus_1c_plus_24_plus_25":True,
       "owner_virtual_0x154_nonnull_sets_time_plus_0x28_to_now":True,
       "primary_handle_re_resolved_twice_after_guard":True,
       "primary_entity_byte_plus_0x3b9_zero_bypasses_timer_and_returns_not_flag26":True,
       "if_time28_zero_and_elapsed_ge_4_sets_byte26_and_returns_false":True,
       "otherwise_nonzero_byte26_signals_owner_condition_0x18_then_clears_byte26":True,
       "finite_clock_stable_handle_host_model_only":True,
       "original_NaN_x87_parity_semantics_ported":False,
       "stale_second_handle_original_safe":False,
       "copyrighted_executable_bytes_exported":False
    }

def self_test():
    # Exact native float/time boundary and marker side effects for ordered input.
    assert model_ordered_finite(2,3.999,0,0,True,False)==(True,0,0,False)
    assert model_ordered_finite(2,4.0,0,0,True,False)==(False,0,1,False)
    assert model_ordered_finite(2,4.1,0,0,True,False)==(False,0,1,False)
    assert model_ordered_finite(2,20,1,1,True,False)==(True,1,0,True)
    assert model_ordered_finite(2,20,1,0,True,False)==(True,1,0,False)
    assert model_ordered_finite(2,20,0,1,True,True)==(True,20,0,True)
    assert model_ordered_finite(2,20,0,1,False,False)==(False,0,1,False)
    assert model_ordered_finite(2,20,0,0,False,False)==(True,0,0,False)
    assert not model_ordered_finite(5,4.9,0,0,True,False)[0]
    for site,target in DIRECT.items():
        displacement=struct.pack("<i",target-site-5)
        assert site+5+struct.unpack("<i",displacement)[0]==target
        for pos in range(4):
            bad=bytearray(displacement);bad[pos]^=1
            assert site+5+struct.unpack("<i",bad)[0]!=target
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError("Accepted incorrect original retail DLL")
    print("PHASE610_ORIGINAL_FREE_ASSAULT_X87_TIMER_STATE_PASS")

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--self-test',action='store_true')
    ap.add_argument('--bin-zip',type=Path)
    ap.add_argument('--server',type=Path)
    ap.add_argument('--client',type=Path)
    ap.add_argument('--out',type=Path)
    args=ap.parse_args()
    if args.self_test:
        self_test()
        if not(args.bin_zip or args.server):return
    if not(args.bin_zip or (args.server and args.client)):
        ap.error("Use --bin-zip, both --server/--client, or --self-test")
    try:
        if args.bin_zip:
            with zipfile.ZipFile(args.bin_zip) as z:
                data=audit(z.read('server.dll'),z.read('client.dll'))
        else:data=audit(args.server.read_bytes(),args.client.read_bytes())
    except (Refusal,OSError,KeyError,struct.error,zipfile.BadZipFile) as e:
        ap.exit(2,"PHASE610 SOURCE AUDIT REFUSED: %s\n"%e)
    output=json.dumps(data,indent=2)+"\n"
    if args.out:
        args.out.write_text(output,encoding="utf-8")
        print("Authenticated original Assault finite-timer stages:",args.out)
    else:print(output)
if __name__=="__main__":main()
