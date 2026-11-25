#!/usr/bin/env python3
"""Owner-only numerical differential. Linux x86_64, binutils, g++.
Runs two authenticated standalone original x87 routines in long mode.
Only absolute constant displacements and return are adapted; this does
not load the retail PE32 DLL or execute any engine/OS-dependent routine.
"""
import argparse
import ctypes as C
import hashlib
import itertools
import mmap
import platform
import random
import re
import struct
import subprocess
import tempfile
from pathlib import Path

SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
BASE=0x24000000
ROOT=Path(__file__).resolve().parents[2]

def f32(x):
    return struct.unpack("<f",struct.pack("<f",x))[0]

def original(path,data,start,end,nargs,expected_relocs):
    raw=bytearray(data[start-BASE:end-BASE])
    if not raw or raw[-1]!=0xc3:
        raise RuntimeError("unexpected function boundary")
    raw[-1]=0x90 # fall through to native x64 return shim
    pro=bytearray(b"\x48\x83\xec\x28")
    for i in range(nargs):
        # movss [rsp+4*(i+1)], xmm<i>; fake PE32 argument slots.
        pro+=bytes((0xf3,0x0f,0x11,0x44+8*i,0x24,4*(i+1)))
    epilogue=b"\xd9\x5c\x24\x20\xf3\x0f\x10\x44\x24\x20\x48\x83\xc4\x28\xc3"
    code=pro+raw+epilogue
    dis=subprocess.check_output(["objdump","-d","-Mintel",f"--start-address={start:#x}",f"--stop-address={end:#x}",str(path)],text=True)
    n=0
    for line in dis.splitlines():
        m=re.match(r"\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*.*ds:0x([0-9a-f]+)",line)
        if not m:
            continue
        addr=int(m[1],16)
        ins=bytes.fromhex(m[2])
        target=int(m[3],16)
        if len(ins)!=6 or ins[0] not in (0xd8,0xd9) or ins[1]&0xc7!=5 or not BASE<=target<=BASE+len(data)-4:
            raise RuntimeError("unexpected absolute reference: "+line)
        pos=len(pro)+addr-start
        off=len(code)
        code+=data[target-BASE:target-BASE+4]
        # Original absolute operand is RIP-relative in long mode.
        struct.pack_into("<i",code,pos+2,off-(pos+6))
        n+=1
    if n!=expected_relocs:
        raise RuntimeError(f"unexpected relocation count {n}")
    mem=mmap.mmap(-1,len(code),prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
    mem.write(code)
    ptr=C.addressof(C.c_char.from_buffer(mem))
    fn=C.CFUNCTYPE(C.c_float,*([C.c_float]*nargs))(ptr)
    print(f"{start:#x}: {len(raw)} original bytes, {n} constant relocations",flush=True)
    return fn,mem

def compare(name,actual,expected,nargs,rng):
    cases=[[f32(rng.uniform(-2,2))]+[f32(rng.random()) for _ in range(nargs-1)] for _ in range(10000)]
    cases.extend([f32(t),*vals] for t in (-1,0,.25,.5,.75,1,2)
                 for vals in itertools.product((0,.125,.25,.5,.75,.875,1),repeat=nargs-1))
    mismatches=0
    for args in cases:
        a,b=actual(*args),expected(*args)
        if struct.pack("<f",a)!=struct.pack("<f",b):
            if mismatches<10:
                print(f"{name} mismatch: {args}: original={a}, rebuilt={b}")
            mismatches+=1
    print(f"{name}: {len(cases)} cases; {mismatches} bitwise mismatches",flush=True)
    return mismatches

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client",type=Path)
    args=parser.parse_args()
    if platform.system()!="Linux" or platform.machine() not in ("x86_64","AMD64"):
        parser.error("Linux x86_64 required")
    path=args.client.resolve()
    data=path.read_bytes()
    if hashlib.sha256(data).hexdigest()!=SHA:
        parser.error("client.dll does not match the authenticated owner PE32")
    # Restrict execution to the two audited, import-free routine slices.
    with tempfile.TemporaryDirectory(prefix="sin-overlay-cycle-") as tmp:
        tmp=Path(tmp)
        source=tmp/"probe.cpp"
        library=tmp/"probe.so"
        source.write_text(r'''
#include "client_overlay_cycle_math.h"
#include <limits>
using namespace sin_port::overlay_cycle_math;
extern "C" float original_loop_linear(float t,float a,float b){float out=0;return linear(t,a,b,out)?out:std::numeric_limits<float>::quiet_NaN();}
extern "C" float original_loop_cubic(float t,float a,float b,float c){float out=0;return cubic(t,a,b,c,true,out)?out:std::numeric_limits<float>::quiet_NaN();}
''',encoding="utf-8")
        subprocess.run(["g++","-O2","-ffp-contract=off","-std=c++17","-shared","-fPIC",
                        "-I",str(ROOT/"pc/independent_slot0"),str(source),"-o",str(library)],check=True)
        cpp=C.CDLL(str(library))
        linear=cpp.original_loop_linear
        linear.argtypes=[C.c_float]*3
        linear.restype=C.c_float
        cubic=cpp.original_loop_cubic
        cubic.argtypes=[C.c_float]*4
        cubic.restype=C.c_float
        orig_linear,mem1=original(path,data,0x2408afb0,0x2408b03c,3,6)
        orig_cubic,mem2=original(path,data,0x2408c440,0x2408c554,4,10)
        try:
            rng=random.Random(1012)
            fails=compare("linear",orig_linear,linear,3,rng)
            fails+=compare("Hermite",orig_cubic,cubic,4,rng)
        finally:
            mem1.close()
            mem2.close()
    return int(fails!=0)

if __name__=="__main__":
    raise SystemExit(main())
