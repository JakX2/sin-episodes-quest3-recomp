"""Original-free, fail-closed PE32 tests for Jessica's ballistic audit."""
import importlib.util
from pathlib import Path
import struct
import sys
script=Path(__file__).resolve().parents[1]/"tools"/"audit_sin_original_jessica_toss.py"
sys.path.insert(0,str(script.parent))
spec=importlib.util.spec_from_file_location("jessica_toss_audit",script)
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def fake_pe():
    b=bytearray(0x340)
    b[:2]=b"MZ"
    struct.pack_into("<I",b,0x3c,0x80)
    b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<H",b,0x86,1)
    struct.pack_into("<H",b,0x94,0xE0)
    struct.pack_into("<H",b,0x98,0x10B)
    struct.pack_into("<I",b,0x98+28,0x22000000)
    struct.pack_into("<IIII",b,0x98+0xE0+8,0x40,0x1000,0x40,0x280)
    b[0x280:0x285]=b"\xE8\xFB\x0F\x00\x00"
    return b
def rejects(fn):
    try: fn()
    except ValueError: return
    raise AssertionError("invalid synthetic input accepted")

def test():
    b=fake_pe()
    pe=audit.PE32(b)
    assert pe.read(0x22001000,5)==b"\xE8\xFB\x0F\x00\x00"
    assert pe.branch_target(0x22001000,0xE8)==0x22002000
    rejects(lambda:audit.verify(b))
    rejects(lambda:pe.read(0x2200103F,2))
    rejects(lambda:pe.read(0x22000000,1))
    rejects(lambda:pe.branch_target(0x22001000,0xE9))
    for at,val in ((0,0),(0x80,0),(0x98,0x0C)):
        broken=fake_pe();broken[at]=val
        rejects(lambda:audit.PE32(broken))
    broken=fake_pe()
    struct.pack_into("<I",broken,0x98+0xE0+20,0xFFFFFFFF)
    rejects(lambda:audit.PE32(broken))
    assert len(audit.WITNESSES)==4
    assert len(audit.CALLS)==5

if __name__=="__main__":
    test()
    print("Original-free Jessica toss audit regression PASS")
