#!/usr/bin/env python3
"""Phase639: authenticate complete retail SiN 2006 allocation methods.

The --self-test uses invented bytes only; no retail DLL is uploaded to CI.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA256="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA256="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
METHODS=(
 ("symbol_slot_register",0x22282270,102,
  "7bf859a991bd9008ccff924d24c8aabb677a8ce8a2452cfeaf61c5b51a31e014"),
 ("acquire_free_or_new_slot",0x222810e0,98,
  "0e31663049d5343559829cd95e2a8ab56ee5d5509d9393bedb615d26cd783e05"),
)
EDGES=(
 (0x2228228b,0x22280fb0,"enumerate_and_rewrite_stack_arguments"),
 (0x22282292,0x222810e0,"acquire_slot"),
 (0x222822a6,0x22281f30,"link_slot"),
 (0x222810fc,0x22280dd0,"grow_slot_array"),
 (0x22281113,0x22280d40,"get_free_slot_next"),
 (0x222823a0,0x22282270,"Phase638_symbol_forwarder_calls_registrar"),
)
# Separate original instruction byte witnesses, no original whole-method
# blobs copied to source. Each witness includes a native VA.
PINS=(
 (0x22282270,"8b442408"),  # arg2 flag loaded before pushes
 (0x22282274,"53"),
 (0x22282275,"8b5c2408"),  # source record ptr EBX
 (0x22282279,"56"),
 (0x2228227a,"57"),
 (0x2228227b,"8bf1"),
 (0x2228227d,"50"),
 (0x2228227e,"8d4c2418"),  # pointer to caller's arg2 word
 (0x22282283,"8d542418"),  # pointer to caller's arg1 word
 (0x22282288,"53"),
 (0x2228228b,"e820edffff"),
 (0x22282290,"8bce"),
 (0x22282292,"e849eeffff"),
 (0x22282297,"8b4c2410"),  # re-read mutable arg1 AFTER enumeration
 (0x2228229b,"8bf8"),     # save handle before linking
 (0x2228229d,"8b442414"),  # re-read mutable arg2 AFTER enumeration
 (0x222822a1,"50"),
 (0x222822a2,"51"),
 (0x222822a3,"57"),
 (0x222822a6,"e885fcffff"),
 (0x222822ab,"8b4604"),  # post-link [table+4] read
 (0x222822ae,"6683461201"),  # count[+12] 16-bit increment
 (0x222822b3,"0fb7d7"),  # WORD handle
 (0x222822b6,"c1e204"),  # 16-byte slots
 (0x222822b9,"8d440208"),  # slot +8
 (0x222822bd,"85c0"),
 (0x222822bf,"740c"),
 (0x222822c1,"668b0b"),  # original entry WORD
 (0x222822c4,"668908"),  # slot+8 WORD
 (0x222822c7,"8b5304"),  # original entry +4 DWORD
 (0x222822ca,"895004"),  # slot+0xC DWORD
 (0x222822cd,"668bc7"),
 (0x222822d3,"c20800"),
 (0x222810e0,"56"),
 (0x222810e3,"668b4614"),  # free-list head
 (0x222810e7,"663dffff"),
 (0x222810ec,"7520"),
 (0x222810ee,"0fb74616"),  # next unused WORD
 (0x222810f2,"394608"),  # capacity comparison
 (0x222810f5,"750a"),
 (0x222810f7,"6a01"),
 (0x222810f9,"8d4e04"),
 (0x222810fc,"e8cffcffff"),
 (0x22281101,"668b7e16"),  # re-read next after growth
 (0x22281105,"8d4f01"),
 (0x22281108,"66894e16"),  # increment WORD, wrap
 (0x2228110e,"50"),
 (0x22281113,"e828fcffff"),
 (0x22281118,"668b5002"),  # next free link
 (0x2228111c,"66895614"),  # new free-list head
 (0x22281120,"8b4e04"),  # slot ptr
 (0x22281123,"0fb7c7"),  # zero extend 16-bit index
 (0x22281126,"c1e004"),
 (0x22281129,"8d440808"),
 (0x2228112d,"85c0"),
 (0x2228112f,"7405"),
 (0x22281131,"66c700ffff"),  # clear slot+8 link
 (0x22281136,"8b5604"),  # refresh slot pointer
 (0x22281139,"668bc7"),
 (0x2228113d,"895618"),  # cache ptr +0x18
 (0x22281141,"c3"),
)
class VerificationError(ValueError):pass
class PEImage:
    def __init__(self,data:bytes):
        self.data=data
        if len(data)<0x100 or data[:2]!=b"MZ":
            raise VerificationError("not a PE image")
        pe,=struct.unpack_from("<I",data,0x3c)
        if pe+24>len(data) or data[pe:pe+4]!=b"PE\x00\x00":
            raise VerificationError("bad PE signature")
        machine,nsects,_,_,_,opsize,_=struct.unpack_from("<HHIIIHH",data,pe+4)
        if machine!=0x14c:
            raise VerificationError("expected x86 PE32")
        optional=pe+24
        if optional+opsize>len(data) or struct.unpack_from("<H",data,optional)[0]!=0x10b:
            raise VerificationError("bad PE32 optional header")
        self.base,=struct.unpack_from("<I",data,optional+28)
        self.sections=[]
        off=optional+opsize
        for i in range(nsects):
            x=off+40*i
            if x+40>len(data):raise VerificationError("truncated section table")
            name=data[x:x+8].split(b"\x00")[0].decode("ascii","replace")
            virtual_size,virtual_address,raw_size,raw_offset=struct.unpack_from("<IIII",data,x+8)
            self.sections.append((name,virtual_address,virtual_size,raw_size,raw_offset))
    def _section(self,va:int,n:int):
        rva=va-self.base
        for name,vaddr,vsize,rawsize,fileoff in self.sections:
            if rva>=vaddr and 0<=n and rva+n<=vaddr+rawsize and (
                rva+n<=vaddr+max(vsize,rawsize)):
                where=fileoff+rva-vaddr
                if where+n<=len(self.data):return name,where
        raise VerificationError(f"unmapped original VA or unbacked bytes: {va:#x} size {n}")
    def section(self,va:int)->str:
        return self._section(va,1)[0]
    def at(self,va:int,n:int)->bytes:
        return self.data[self._section(va,n)[1]:self._section(va,n)[1]+n]
def check_full(read,va,size,sha):
    body=read(va,size)
    if len(body)!=size or hashlib.sha256(body).hexdigest()!=sha:
        raise VerificationError(f"wrong complete original method {va:#x}")
def check_edge(read,site,target):
    code=read(site,5)
    if len(code)!=5 or code[0]!=0xe8 or (
        site+5+struct.unpack_from("<i",code,1)[0]!=target):
        raise VerificationError(f"wrong direct call {site:#x} to {target:#x}")
def check_pin(read,va,hexbytes):
    payload=bytes.fromhex(hexbytes)
    if read(va,len(payload))!=payload:
        raise VerificationError(f"wrong x86 opcode witness {va:#x}")
def authenticate(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("wrong retail 2006 server.dll/client.dll identity")
    pe=PEImage(server)
    if pe.base!=0x22000000:raise VerificationError("wrong native image base")
    for _,va,n,sha in METHODS:
        if pe.section(va)!=".text":raise VerificationError("method outside .text")
        check_full(pe.at,va,n,sha)
    for site,target,_ in EDGES:
        if pe.section(site)!=".text" or pe.section(target)!=".text":
            raise VerificationError("direct call outside original .text")
        check_edge(pe.at,site,target)
    for va,hexbytes in PINS:check_pin(pe.at,va,hexbytes)
    return {
      "status":"PHASE639_RETAIL_2006_SLOT_REGISTRATION_VERIFIED",
      "original_methods":[{"name":name,"va":hex(va),"bytes":n,"sha256":sha}
                          for name,va,n,sha in METHODS],
      "complete_executable_bytes":sum(x[2] for x in METHODS),
      "direct_edges": [{"site":hex(s),"target":hex(t),"purpose":p}
                       for s,t,p in EDGES],
      "opcode_witnesses":len(PINS),
      "enumerator_fully_recovered":False,
      "link_repair_fully_recovered":False,
      "free_slot_reader_fully_recovered":False,
      "capacity_grower_fully_recovered":False,
      "actual_original_gameplay_executed":False
    }
def self_test():
    made=bytes((17*i+9)&255 for i in range(200))
    digest=hashlib.sha256(made).hexdigest()
    check_full(lambda _va,n:made[:n],0x71001000,len(made),digest)
    for i in range(len(made)):
        mutant=bytearray(made);mutant[i]^=1
        try:check_full(lambda _va,n:bytes(mutant[:n]),0x71001000,len(made),digest)
        except VerificationError:pass
        else:raise AssertionError("corrupt full-body bytes accepted")
    site,target=0x71000000,0x71001000
    edge=b"\xe8"+struct.pack("<i",target-site-5)
    check_edge(lambda _va,n:edge[:n],site,target)
    for i in range(5):
        bad=bytearray(edge);bad[i]^=1
        try:check_edge(lambda _va,n:bytes(bad[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError("wrong edge accepted")
    check_pin(lambda _va,n:b"\x66\x83\x46\x12\x01"[:n],site,"6683461201")
    try:check_pin(lambda _va,n:b"\x83\x46\x12\x01"[:n],site,"6683461201")
    except VerificationError:pass
    else:raise AssertionError("wrong 16-bit counter width accepted")
    try:authenticate(b"wrong",b"wrong")
    except VerificationError:pass
    else:raise AssertionError("wrong retail identities accepted")
    print("PASS Phase639 invented SHA256/rel32/opcode/counter-width mutation and wrong-DLL refusal")
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    parser.add_argument("--output",type=Path)
    parser.add_argument("--self-test",action="store_true")
    a=parser.parse_args()
    if a.self_test:
        self_test();return
    if not a.server or not a.client:
        parser.error("specify both --server and --client, or --self-test")
    try:report=authenticate(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        parser.exit(1,"PHASE639 ORIGINAL AUDIT REFUSED: "+str(exc)+"\n")
    text=json.dumps(report,indent=2)+"\n"
    if a.output:a.output.write_text(text,encoding="utf-8")
    else:print(text,end="")
if __name__=="__main__":main()
