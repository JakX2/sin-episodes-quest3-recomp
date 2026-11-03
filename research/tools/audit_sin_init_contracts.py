#!/usr/bin/env python3
"""Validate original SiN 2006 client/server early Init() dependency handshake.

The prior Phase 84 audit proves exact owner-held DLL SHA256, CreateInterface
registries and vtables. Emits metadata only, never original executable code.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_factories import PE32, VerificationError, audit as original_factory_audit

INIT = {
    "client.dll": {
        "start":0x240fe0e0,"end":0x240fe5a4,"fail":0x240fe59e,
        "sha256":"14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db",
        "requirements":(
                ("VEngineClient012", 0x240fe115, "esi"),
                ("VEngineModel012", 0x240fe12e, "esi"),
                ("VEngineEffects001", 0x240fe147, "esi"),
                ("VEngineCvar002", 0x240fe160, "esi"),
                ("EngineTraceClient003", 0x240fe179, "esi"),
                ("VEngineRenderView011", 0x240fe192, "esi"),
                ("VDebugOverlay003", 0x240fe1ab, "esi"),
                ("VMaterialSystem076", 0x240fe1c4, "esi"),
                ("VEngineCache001", 0x240fe1dd, "esi"),
                ("VModelInfoClient003", 0x240fe1f6, "esi"),
                ("VEngineVGui001", 0x240fe20f, "esi"),
                ("VEngineClientStringTable001", 0x240fe228, "esi"),
                ("SpatialPartition001", 0x240fe241, "esi"),
                ("VEngineShadowMgr002", 0x240fe25a, "esi"),
                ("StaticPropMgrClient004", 0x240fe273, "esi"),
                ("IEngineSoundClient003", 0x240fe28c, "esi"),
                ("VFileSystem012", 0x240fe2a5, "esi"),
                ("VEngineRandom001", 0x240fe2be, "esi"),
                ("VENGINE_GAMEUIFUNCS_VERSION004", 0x240fe2d7, "esi"),
                ("GAMEEVENTSMANAGER002", 0x240fe2f0, "esi"),
                ("VSoundEmitter001", 0x240fe309, "esi"),
        )},
    "server.dll": {
        "start":0x221e7640,"end":0x221e79ab,"fail":0x221e79a1,
        "sha256":"abb6eb2fc523fc0cb6301630e7ba9f0b61e70e48f15d5809145171250898b680",
        "requirements":(
                ("VEngineServer021", 0x221e7650, "esi"),
                ("VoiceServer001", 0x221e7669, "esi"),
                ("VEngineCvar002", 0x221e7682, "esi"),
                ("VEngineServerStringTable001", 0x221e769b, "esi"),
                ("StaticPropMgrServer002", 0x221e76b4, "esi"),
                ("VEngineRandom001", 0x221e76cd, "esi"),
                ("IEngineSoundServer003", 0x221e76e6, "esi"),
                ("SpatialPartition001", 0x221e76ff, "esi"),
                ("VModelInfoServer001", 0x221e7718, "esi"),
                ("EngineTraceServer003", 0x221e7731, "esi"),
                ("VFileSystem012", 0x221e774e, "edi"),
                ("GAMEEVENTSMANAGER002", 0x221e7767, "esi"),
                ("VEngineCache001", 0x221e7780, "esi"),
                ("VSoundEmitter001", 0x221e7799, "esi"),
                ("ServerUploadGameStats001", 0x221e77b2, "esi"),
                ("ISERVERPLUGINHELPERS001", 0x221e77cb, "esi"),
        )},
}
# Additional, separate post-core virtual +0 gate on the sound emitter.
LATE = {
    "client.dll":{"gate_virtual":0x240fe320,"gate_branch":0x240fe324,
                  "return_true":0x240fe595,"return_false":0x240fe59e,
                  "ret_bytes":12,"optional_query":0x240fe34e},
    "server.dll":{"gate_virtual":0x221e77e6,"gate_branch":0x221e77ea,
                  "return_true":0x221e7978,"return_false":0x221e79a1,
                  "ret_bytes":16,"optional_query":0x221e7949},
}
def check(pe,site,expected):
    if pe.at(site,len(expected))!=expected:
        raise VerificationError('original SiN Init instruction mismatch at %#x'%site)

def audit_init_one(pe,module,verify_region_hash=True):
    spec=INIT[module];late=LATE[module];entries=[]
    if verify_region_hash and hashlib.sha256(pe.at(spec['start'],spec['end']-spec['start'])).hexdigest()!=spec['sha256']:
        raise VerificationError('original SiN Init SHA-256 mismatch: '+module)
    for idx,(expected,site,reg) in enumerate(spec['requirements']):
        push=site-(10 if module=="client.dll" and idx==0 else 5)
        check(pe,push-2,b'\x6a\x00') # nullptr return-code pointer
        check(pe,push,b'\x68')
        ptr=struct.unpack('<I',pe.at(push+1,4))[0]
        if pe.cstring(ptr)!=expected:
            raise VerificationError('original SiN interface name mismatch at %#x'%site)
        check(pe,site,b'\xff'+(b'\xd6' if reg=="esi" else b'\xd7'))
        check(pe,site+2,b'\x83\xc4'+(b'\x28' if module=="client.dll" and idx==0 else b'\x08'))
        check(pe,site+5,b'\x85\xc0')
        check(pe,site+7,b'\xa3')
        dest=struct.unpack('<I',pe.at(site+8,4))[0]
        branch=site+12
        check(pe,branch,b'\x0f\x84'+struct.pack('<i',spec['fail']-branch-6))
        entries.append({"index":idx,"name":expected,
                        "factory_argument":"third_argument" if reg=="edi" else "first_argument",
                        "original_call_va":hex(site),"interface_string_va":hex(ptr),
                        "result_pointer_global_va":hex(dest),
                        "missing_jump_va":hex(spec["fail"])})
    check(pe,late['gate_virtual'],b'\xff'+(b'\x12' if module=="client.dll" else b'\x10'))
    check(pe,late['gate_virtual']+2,b'\x84\xc0')
    jmp=late['gate_branch']
    check(pe,jmp,b'\x0f\x84'+struct.pack('<i',spec['fail']-jmp-6))
    check(pe,late['return_true'],
          b'\xb8\x01\x00\x00\x00' if module=="client.dll" else b'\x5f\xb0\x01')
    check(pe,late['return_false'],
          b'\x33\xc0' if module=="client.dll" else b'\x5f\x32\xc0')
    return_va=0x240fe59b if module=="client.dll" else 0x221e79a8
    check(pe,return_va,b'\xc2'+struct.pack('<H',late['ret_bytes']))
    # Later factory calls are separate from the strict core null-failure run.
    check(pe,late['optional_query'],b'\xff\xd6')
    if module=="server.dll":
        check(pe,0x221e7743,b'\x8b\x7c\x24\x20')
    else:
        check(pe,0x240fe105,b'\x8b\x74\x24\x28')
        check(pe,0x240fe101,b'\x8b\x44\x24\x30')
    return {
        "module":module,"original_vtable_slot_0_va":hex(spec['start']),
        "full_original_init_function_sha256":spec["sha256"] if verify_region_hash else "fixture_without_hash",
        "required_core_count":len(entries),"ordered_required_factory_calls":entries,
        "sound_emitter_virtual_slot_0_gate_va":hex(late["gate_virtual"]),
        "later_nonfatal_factory_call_va":hex(late["optional_query"]),
        "success_return":1,"failure_return":0,
        "observed_callee_stack_cleanup_bytes":late["ret_bytes"],
        "scope":"Only early required factory sequence plus emitter gate; full client/server initialization is not implemented",
    }
def audit(client,server):
    previous=original_factory_audit(client,server)
    return {"prior_original_sha256":{"client.dll":previous["client"]["sha256"],
                                     "server.dll":previous["server"]["sha256"]},
            "client":audit_init_one(PE32(client),"client.dll"),
            "server":audit_init_one(PE32(server),"server.dll")}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"client":result["client"]["required_core_count"],
                      "server":result["server"]["required_core_count"]},indent=2))
if __name__=="__main__":main()
