#!/usr/bin/env python3
"""Read-only original SiN 2006 server cache record + client late shutdown audit.

Chains Phase89 exact original client/server PE32 checks; does not decode
variable-length cache files or publish copyrighted executable binary bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_cache_headers_client_teardown import audit as previous_audit
from audit_sin_original_factories import PE32,VerificationError

REGIONS={
 "client.dll":{
   "client_late_virtual_shutdown_fanout":(0x241ee0c0,120,"f291a3fc3a0d72d08184ce42c4e426e247c6d23b90c47af728d8eb3296e61783"),
   "client_another_global_release":(0x240fc900,27,"71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88"),
 },
 "server.dll":{
   "scene_16bit_record_find_or_insert":(0x221acfd0,190,"48004fea0993479cec22accfbf75523c49b3e7f0cebb5a05ef228e178d4c0804"),
   "modelsounds_16bit_record_find_or_insert":(0x22149770,187,"c79d38ec382e6477cf1872553addccc307629672bcafaf5a5f7e2c22e97b8e49"),
 }
}
SERVER_SITES={
 0x221ad019:"0fb7c0",0x221ad01c:"3dffff0000",0x221ad023:"6a1c",
 0x221ad031:"c700a0dc4222",0x221ad074:"0fb7c0",
 0x221ad07a:"8d1449",0x221ad080:"895cd114",0x221ad08b:"c20400",
 0x221497b9:"0fb7c0",0x221497bc:"3dffff0000",0x221497c3:"6a18",
 0x221497d1:"c700c8ab4022",0x22149811:"0fb7c0",
 0x22149817:"8d1449",0x2214981d:"895cd114",0x22149828:"c20400",
 0x221ad563:"8b4e08",0x221ad566:"0fb7c0",
 0x221ad569:"8d0440",0x221ad56c:"8d6cc108",
 0x221ad579:"8b550c",0x221ad57c:"894504",
 0x221ad57f:"8b4620",0x221ad582:"8b0c90",
 0x221ad585:"8b11",0x221ad58c:"ff5204",
 0x22149a43:"8b4e08",0x22149a46:"0fb7c0",
 0x22149a49:"8d0440",0x22149a4c:"8d6cc108",
 0x22149a59:"8b550c",0x22149a5c:"894504",
 0x22149a5f:"8b4620",0x22149a62:"8b0c90",
 0x22149a65:"8b11",0x22149a6c:"ff5204",
 0x221acfff:"ff90d4000000",0x2214979f:"ff90d4000000",
}
CLIENT_SITES={
 0x240fc900:"8b0df4f33324",0x240fc906:"85c9",0x240fc908:"7406",
 0x240fc90c:"6a01",0x240fc90e:"ff10",
 0x240fc910:"c705f4f3332400000000",0x240fc91a:"c3",
 0x241ee0c5:"8b0d90dc3124",0x241ee0cd:"ff5004",
 0x241ee0d0:"8b0da4de3124",0x241ee0d8:"ff5204",
 0x241ee0db:"8b0d00dd3124",0x241ee0e3:"ff5004",
 0x241ee0e6:"8b0d3cde3124",0x241ee0ee:"ff5204",
 0x241ee0f1:"8b0dd0dd3124",0x241ee0f9:"ff5004",
 0x241ee0fc:"8b0decdb3124",0x241ee104:"ff521c",
 0x241ee107:"8b0d40df3124",0x241ee10f:"ff5004",
 0x241ee112:"8b0dbcee3124",0x241ee11a:"ff5204",
 0x241ee11d:"8b0da0f83724",0x241ee123:"85c9",
 0x241ee125:"7405",0x241ee129:"ff500c",
 0x241ee135:"ff6214",
}
CALLS={
 "client.dll":{
   0x241ee0c0:0x2422ece0,0x241ee12c:0x24001060,
   0x240ff489:0x241ee0c0,0x240ff3d4:0x240fc900,
 },
 "server.dll":{
  0x221ad014:0x221a7fb0,0x221ad025:0x2226e790,
  0x221ad060:0x221a6620,0x221ad06f:0x221acd00,
  0x221497b4:0x22142670,0x221497c5:0x2226e790,
  0x221497fd:0x22140810,0x2214980c:0x22146ef0,
  0x221ad51f:0x2228d7d0,0x221ad53f:0x2228d6a0,
  0x221ad552:0x2228d810,0x221ad55e:0x221acfd0,
  0x221ad574:0x2213acb0,
  0x221499ff:0x2228d7d0,0x22149a1f:0x2228d6a0,
  0x22149a32:0x2228d810,0x22149a3e:0x22149770,
  0x22149a54:0x2213acb0,
 }
}
LATE_CLIENT_GLOBALS=(
 (0x2431dc90,0x04),(0x2431dea4,0x04),
 (0x2431dd00,0x04),(0x2431de3c,0x04),
 (0x2431ddd0,0x04),(0x2431dbec,0x1c),
 (0x2431df40,0x04),(0x2431eebc,0x04)
)
def check(pe,va,data,kind):
    if pe.at(va,len(data))!=data:
        raise VerificationError("Phase90 %s mismatch at %#x"%(kind,va))
def verify_one(pe,module,check_hashes=True):
    regions={}
    for name,(va,length,digest) in REGIONS[module].items():
        if check_hashes and hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
            raise VerificationError("Phase90 original code SHA-256 mismatch: "+name)
        regions[name]={"va":hex(va),"size":length,"sha256":digest if check_hashes else None}
    sites=CLIENT_SITES if module=="client.dll" else SERVER_SITES
    for va,op in sites.items():check(pe,va,bytes.fromhex(op),"opcode")
    for va,target in CALLS[module].items():
        check(pe,va,b"\xe8"+struct.pack("<i",target-va-5),"CALL")
    result={"module":module,"regions":regions,
            "opcode_site_count":len(sites),"call_edge_count":len(CALLS[module])}
    if module=="client.dll":
        result.update({
          "late_shutdown_va":"0x241EE0C0",
          "first_helper_va":"0x2422ECE0",
          "eight_global_virtual_calls":[{"global_va":hex(g),"slot_byte_offset":s}
                                        for g,s in LATE_CLIENT_GLOBALS],
          "optional_global_va":"0x2437F8A0","optional_virtual_byte_offset":12,
          "final_interface_factory_va":"0x24001060",
          "final_virtual_byte_offset":20,
          "extra_optional_global_release_va":"0x240FC900",
          "extra_released_global_va":"0x2433F3F4",
          "extra_virtual_release_byte_offset":0,
          "extra_virtual_release_arg":1,
        })
    else:
        result.update({
          "scene_record_insert_va":"0x221ACFD0",
          "modelsounds_record_insert_va":"0x22149770",
          "not_found_index":65535,
          "record_index_width_bits":16,
          "scene_allocated_helper_bytes":28,
          "modelsounds_allocated_helper_bytes":24,
          "in_memory_record_stride_bytes":24,
          "record_reader_view_offset":8,
          "record_output_pointer_offset_from_array_base":12,
          "record_handler_index_offset_from_array_base":20,
          "record_insert_handler_store_offset_from_array_base":20,
          "record_handler_virtual_byte_offset":4,
          "record_loop_count":"fourth header field, signed; loop only for count > 0",
          "disk_record_schema":"NOT recovered; on-disk field count and string encoding unverified",
        })
    result["scope"]="x86 binary exact layout/call order only; not an Android game module or full cache payload decoder"
    return result
def audit(client,server):
    previous_audit(client,server)
    return {"client":verify_one(PE32(client),"client.dll"),
            "server":verify_one(PE32(server),"server.dll")}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    report=audit(args.client.read_bytes(),args.server.read_bytes())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({m:{"regions":len(report[m]["regions"]),
                          "sites":report[m]["opcode_site_count"],
                          "calls":report[m]["call_edge_count"]}
        for m in ("client","server")},indent=2))
if __name__=="__main__": main()
