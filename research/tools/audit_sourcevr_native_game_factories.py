#!/usr/bin/env python3
"""Audit SHA-pinned SiN PE32 vs actual SourceVR 0.5.5 ARM64 client/server factory registrations.

Requires user's private original bin.zip and a rejoined SourceVR multipart ZIP.
No proprietary bytes are generated, committed or printed. JSON metadata only.
"""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path

ORIGINAL = {
    "server.dll": ("568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650", "ServerGameDLL003"),
    "client.dll": ("6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d", "VClient011"),
}
NAMES = {"libserver.so": "ServerGameDLL010", "libclient.so": "VClient017"}
PROFILES = ("hl2", "ep1", "ep2", "lostcoast", "portal", "portal2", "entropyzero")
# 2006-vs-2026 binary fingerprints, not a writable compatibility patch.
# SHA256 -> (module, string VA, ADRP x2 registration VA, registrar VA, CreateInterface VA)
WITNESSES = {
    "fdc8bb8777e044875609da75850fdf0a58155d5caf84b834a9d72486fe0a2322": ("libserver.so", 0xa4291, 0x532a68, 0x94aee0, 0x94af80),
    "a81b04bdb0164ee450891a153c055efd268562b7171181f1e83f401b64c7c226": ("libserver.so", 0xae196, 0x551f48, 0x9cdfc0, 0x9ce060),
    "6c8b6df1db323288e3c49b635569006b6e3cfbfc7745b443c655d5ed4e31e8d8": ("libserver.so", 0xa1034, 0x44c6e8, 0x922600, 0x9226a0),
    "ecb111cb112f0388f075382fdf15cd8105c2472cac0a478aa073c31a2353f87e": ("libserver.so", 0xb5904, 0x4935c8, 0xa50560, 0xa50600),
    "16e461d91ab54cc9491081363e9a801bb20e915e5c44a272a1de524e51b15c8a": ("libserver.so", 0xaf74d, 0x556aa8, 0x9dce00, 0x9dcea0),
    "0a8d1082e86d857cad812a65ca682695098de2498919931b08e87fb3b38bc362": ("libclient.so", 0x49489, 0x370848, 0x722360, 0x722400),
    "e4384399bd36ebeec04c678d9d757176935e4d7ca3d228bcded5536aabb50c37": ("libclient.so", 0x49b0a, 0x3745c8, 0x72ee40, 0x72eee0),
    "9ad8e9eff680ccd4a29f46d540f4d737bfb9467c3013b9ca93d123834f50c773": ("libclient.so", 0x49af9, 0x3745c8, 0x72ee40, 0x72eee0),
    "f14083ceaeb46317ab9cd1cdf36e011a6d4013462bc7143403dcc8152ae53b9b": ("libclient.so", 0x480d6, 0x2d1aa4, 0x6fa6e0, 0x6fa780),
    "0f555f09a2ab90c89501a8aa8c7a153ed2c7a5d6906816f8779e3ff3bf222ae2": ("libclient.so", 0x4e04b, 0x2ec6a4, 0x7700e0, 0x770180),
    "720602b9643c4b2505ba9c7f6e5e0ac05fcb99270965a5b74b3c26d3e89ca1b0": ("libclient.so", 0x49de5, 0x3755a8, 0x7319c0, 0x731a60),
}
# Explicitly pin which game profile chooses each native server/client pair.
PROFILE_SHAS = {
    "hl2": ("fdc8bb8777e044875609da75850fdf0a58155d5caf84b834a9d72486fe0a2322", "0a8d1082e86d857cad812a65ca682695098de2498919931b08e87fb3b38bc362"),
    "ep1": ("a81b04bdb0164ee450891a153c055efd268562b7171181f1e83f401b64c7c226", "e4384399bd36ebeec04c678d9d757176935e4d7ca3d228bcded5536aabb50c37"),
    "ep2": ("a81b04bdb0164ee450891a153c055efd268562b7171181f1e83f401b64c7c226", "9ad8e9eff680ccd4a29f46d540f4d737bfb9467c3013b9ca93d123834f50c773"),
    "lostcoast": ("fdc8bb8777e044875609da75850fdf0a58155d5caf84b834a9d72486fe0a2322", "0a8d1082e86d857cad812a65ca682695098de2498919931b08e87fb3b38bc362"),
    "portal": ("6c8b6df1db323288e3c49b635569006b6e3cfbfc7745b443c655d5ed4e31e8d8", "f14083ceaeb46317ab9cd1cdf36e011a6d4013462bc7143403dcc8152ae53b9b"),
    "portal2": ("ecb111cb112f0388f075382fdf15cd8105c2472cac0a478aa073c31a2353f87e", "0f555f09a2ab90c89501a8aa8c7a153ed2c7a5d6906816f8779e3ff3bf222ae2"),
    "entropyzero": ("16e461d91ab54cc9491081363e9a801bb20e915e5c44a272a1de524e51b15c8a", "720602b9643c4b2505ba9c7f6e5e0ac05fcb99270965a5b74b3c26d3e89ca1b0"),
}

class AuditError(ValueError):
    pass

def require(ok, message):
    if not ok:
        raise AuditError(message)

def word(data, off):
    require(0 <= off <= len(data)-4, "out-of-bounds instruction")
    return struct.unpack_from("<I", data, off)[0]

def decode_adrp(va, inst, reg):
    require(inst & 0x9f00001f == (0x90000000 | reg), f"ADRP x{reg} changed at {va:#x}")
    disp = (((inst>>5)&0x7ffff)<<2) | ((inst>>29)&3)
    if disp & (1<<20):
        disp -= 1<<21
    return (va & ~4095) + (disp<<12)

def decode_add(inst, reg):
    require(inst & 0xffc003ff == (0x91000000 | (reg<<5) | reg),
            f"ADD x{reg},x{reg} changed")
    return (inst>>10)&4095

def decode_bl(va, inst):
    require(inst & 0xfc000000 == 0x94000000, f"BL changed at {va:#x}")
    delta=inst & 0x3ffffff
    if delta & (1<<25):
        delta-=1<<26
    return va+(delta<<2)

def decode_ldrx(inst, dest, base):
    require(inst & 0xffc003ff == (0xf9400000 | (base<<5) | dest), "LDR changed")
    return ((inst>>10)&4095)*8

def decode_strx(inst, src, base):
    require(inst & 0xffc003ff == (0xf9000000 | (base<<5) | src), "STR changed")
    return ((inst>>10)&4095)*8

class NativeELF:
    """Small read-only ELF64 section/dynamic-symbol reader; no native calls."""
    def __init__(self, data):
        self.data=data
        require(data[:6]==b"\x7fELF\x02\x01", "expected ELF64 little endian")
        require(struct.unpack_from("<HH",data,16)==(3,183), "expected AArch64 ET_DYN")
        shoff=struct.unpack_from("<Q",data,40)[0]
        size=struct.unpack_from("<H",data,58)[0]
        count=struct.unpack_from("<H",data,60)[0]
        idx=struct.unpack_from("<H",data,62)[0]
        require(size==64 and 3<=count<=256 and idx<count and
                shoff+count*size<=len(data), "invalid section table")
        sections=[struct.unpack_from("<IIQQQQIIQQ",data,shoff+n*size) for n in range(count)]
        shstr=sections[idx]
        names=data[shstr[4]:shstr[4]+shstr[5]]
        self.sections={}
        for s in sections:
            require(s[1]==8 or s[4]+s[5]<=len(data), "invalid section bounds")
            require(s[0]<len(names), "invalid section name")
            end=names.find(b"\x00",s[0])
            require(end>=0, "unterminated section name")
            name=names[s[0]:end].decode("ascii","replace")
            self.sections[name]=s
        require(all(x in self.sections for x in (".text",".rodata",".dynsym")),
                "required ELF sections absent")
        dyn=self.sections[".dynsym"]
        require(dyn[9]==24 and dyn[5]%24==0 and dyn[6]<len(sections),
                "invalid dynamic symbol table")
        strings_section=sections[dyn[6]]
        names=data[strings_section[4]:strings_section[4]+strings_section[5]]
        self.exports={}
        for off in range(dyn[4],dyn[4]+dyn[5],24):
            key, info, other, section, va, n=struct.unpack_from("<IBBHQQ",data,off)
            require(key<len(names),"invalid .dynstr index")
            end=names.find(b"\x00",key)
            require(end>=0,"unterminated .dynstr")
            sym=names[key:end].decode("ascii","replace")
            if sym and section and info>>4:
                self.exports[sym]=va

    def at(self,va,size):
        for s in self.sections.values():
            if s[1]==1 and s[3]<=va and va+size<=s[3]+s[5]:
                off=s[4]+va-s[3]
                return self.data[off:off+size]
        raise AuditError(f"unreadable ELF VA {va:#x}")

    def word(self,va):
        return word(self.at(va,4),0)

    def flags(self,va):
        for s in self.sections.values():
            if s[3]<=va<s[3]+s[5]:
                return s[2]
        return 0

def verify_registration(elf,kind,name_va,xref,registrar,create):
    label=NAMES[kind]
    require(elf.at(name_va,len(label)+1)==label.encode()+b"\x00",
            "factory registration string changed")
    require(elf.exports.get("CreateInterface")==create,
            "exported CreateInterface address changed")
    require(elf.flags(name_va)&2, "factory name not allocated")

    # At factory registration call: x0=node, x1=callback, x2=literal name.
    def latest_pair(reg):
        found=[]
        for va in range(xref-36,xref-4,4):
            try:
                found.append(decode_adrp(va,elf.word(va),reg) +
                             decode_add(elf.word(va+4),reg))
            except AuditError:
                pass
        require(bool(found),f"missing x{reg} factory registration setup")
        return found[-1]
    node,callback=latest_pair(0),latest_pair(1)
    string=decode_adrp(xref,elf.word(xref),2)+decode_add(elf.word(xref+4),2)
    require(string==name_va, "factory name is not passed to registration")
    require(decode_bl(xref+8,elf.word(xref+8))==registrar, "wrong registrar call")
    require(elf.flags(node)&1,"registration node must be writable")
    require(elf.flags(callback)&4,"factory callback must be executable")

    # Callback supplies singleton; registrar forms 24-byte linked registry.
    singleton=(decode_adrp(callback,elf.word(callback),0) +
               decode_add(elf.word(callback+4),0))
    require(elf.word(callback+8)==0xd65f03c0,"callback return changed")
    require(elf.flags(singleton)&1,"factory singleton is not writable")
    require(elf.word(registrar+4)==0xa9000801, "factory callback/name STP changed")
    head=decode_adrp(registrar,elf.word(registrar),8)
    offset=decode_ldrx(elf.word(registrar+8),9,8)
    require(decode_strx(elf.word(registrar+12),0,8)==offset,
            "factory head load/store mismatch")
    head+=offset
    require(elf.word(registrar+16)==0xf9000809 and
            elf.word(registrar+20)==0xd65f03c0,"factory next pointer changed")

    # The export walks the SAME linked registry and compares names.
    cihead=(decode_adrp(create+16,elf.word(create+16),8) +
            decode_ldrx(elf.word(create+24),21,8))
    require(cihead==head,"CreateInterface uses another registry")
    for off, expected in (
        (36,0xf94006a0),(40,0xaa1403e1),(48,0x34000160),
        (52,0xf9400ab5),(56,0xb5ffff75),
        (100,0xf94002a0),(116,0xd61f0000)):
        require(elf.word(create+off)==expected,
                f"CreateInterface lookup changed at +{off:#x}")
    decode_bl(create+44,elf.word(create+44))
    return {
        "module":kind, "registered_name":label,
        "string_va":f"0x{name_va:x}", "registration_call_va":f"0x{xref+8:x}",
        "registrar_va":f"0x{registrar:x}", "factory_callback_va":f"0x{callback:x}",
        "singleton_va":f"0x{singleton:x}",
        "registry_head_va":f"0x{head:x}",
        "createinterface_va":f"0x{create:x}",
    }

def audit(original_zip,sourcevr_zip):
    for fname,(sha,name) in ORIGINAL.items():
        with zipfile.ZipFile(original_zip) as archive:
            data=archive.read(fname)
        require(hashlib.sha256(data).hexdigest()==sha,
                f"original {fname} SHA256 mismatch")
        require(name.encode()+b"\x00" in data,
                f"original {fname} required factory missing")
    records={}
    mappings=[]
    with zipfile.ZipFile(sourcevr_zip) as archive:
        members=set(archive.namelist())
        require(not any(n.lower().endswith("/sin.manifest") for n in members),
                "unknown SiN-native profile present; re-audit all compatibility claims")
        for profile in PROFILES:
            path=f"source vr/assets/sourcevr/v1/native/{profile}.manifest"
            selected={}
            for row in archive.read(path).decode().splitlines():
                cells=row.split("\t")
                if len(cells)>=5 and cells[2] in NAMES:
                    kind=cells[2]
                    require(kind not in selected,f"duplicate {kind} in {profile}")
                    sha=cells[1]
                    require(re.fullmatch("[0-9a-f]{64}",sha)!=None,"bad manifest SHA")
                    module_path="source vr/assets/"+cells[4]
                    require(module_path.endswith("/"+kind) and
                            module_path in members,"bad manifest module path")
                    selected[kind]=(sha,module_path)
            expected=dict(zip(("libserver.so","libclient.so"),PROFILE_SHAS[profile]))
            require(set(selected)==set(expected),f"missing game modules for {profile}")
            require(all(selected[k][0]==expected[k] for k in expected),
                    f"{profile} module hashes differ from pinned release")
            mappings.append({"profile":profile,
                             "server_sha256":expected["libserver.so"],
                             "client_sha256":expected["libclient.so"]})
            for kind,(sha,module_path) in selected.items():
                if sha in records:
                    continue
                witness=WITNESSES[sha]
                require(witness[0]==kind,"incorrect native witness kind")
                data=archive.read(module_path)
                require(hashlib.sha256(data).hexdigest()==sha,
                        f"native {kind} binary hash mismatch")
                evidence=verify_registration(NativeELF(data),kind,*witness[1:])
                records[sha]={"sha256":sha,**evidence}
    require(set(records)==set(WITNESSES),"native profile blob coverage changed")
    return {
        "original_hashes":{k:v[0] for k,v in ORIGINAL.items()},
        "original_required_factories":{k:v[1] for k,v in ORIGINAL.items()},
        "sourcevr_native_factories":NAMES,
        "profiles":mappings,
        "native_evidence":list(records.values()),
        "finding":"All 11 SHA-pinned SourceVR client/server modules register factory names different from original SiN; direct factory-name reuse is incompatible.",
        "not_proven":[
            "SourceVR game-module loader accepts custom SiN ARM64 binaries",
            "SourceVR C++ method/vtable ABI matches the old SiN 2006 DLLs",
            "SourceVR contains native SiN factory, working SinTEK NPC, or Quest 3 gameplay",
        ]
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sin-bin-zip",required=True,type=Path)
    p.add_argument("--sourcevr-zip",required=True,type=Path,
                   help='Private JOINED SourceVR ZIP, not only the last split volume')
    a=p.parse_args()
    try:
        print(json.dumps(audit(a.sin_bin_zip,a.sourcevr_zip),indent=2))
    except (AuditError,zipfile.BadZipFile,KeyError,OSError,struct.error) as e:
        p.exit(2,f"Native game-factory audit FAILED: {e}\n")

if __name__=="__main__":
    main()
