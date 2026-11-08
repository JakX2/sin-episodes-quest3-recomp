"""Binary-free synthetic PE32 tests for original absolute velocity audit."""
import importlib.util
from pathlib import Path
import struct
import sys
TOOLS=Path(__file__).resolve().parents[1]/"tools"
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location(
    "abs_velocity_audit",TOOLS/"audit_sin_original_absolute_velocity.py")
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def synthetic_pe():
    buf=bytearray(0x400)
    buf[:2]=b"MZ"
    struct.pack_into("<I",buf,0x3c,0x80)
    buf[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<H",buf,0x86,1)
    struct.pack_into("<H",buf,0x94,0xE0)
    struct.pack_into("<H",buf,0x98,0x10B)
    struct.pack_into("<I",buf,0x98+28,0x22000000)
    struct.pack_into("<IIII",buf,0x98+0xE0+8,
                     0x40,0x1000,0x40,0x280)
    buf[0x280:0x285]=b"\xE8\xFB\x0F\x00\x00"
    return buf
def rejects(callback):
    try:callback()
    except ValueError:return
    raise AssertionError("malformed or nonoriginal image unexpectedly accepted")
def test():
    p=audit.PE32(synthetic_pe())
    assert p.base==0x22000000
    assert p.read(0x22001000,5)==b"\xE8\xFB\x0F\x00\x00"
    assert p.branch_target(0x22001000,0xE8)==0x22002000
    rejects(lambda:p.branch_target(0x22001000,0xE9))
    rejects(lambda:p.read(0x2200103f,2))
    rejects(lambda:audit.verify(synthetic_pe()))
    b=synthetic_pe();b[0]=0
    rejects(lambda:audit.PE32(b))
    b=synthetic_pe();b[0x98]=0x0c
    rejects(lambda:audit.PE32(b))
    b=synthetic_pe()
    struct.pack_into("<I",b,0x98+0xE0+20,0xFFFFFFFF)
    rejects(lambda:audit.PE32(b))
    assert len(audit.HASHES)==6
    assert len(audit.CALLS)==10
    assert len(audit.ANCHORS)==15
if __name__=="__main__":
    test()
    print("Synthetic original absolute velocity audit PASS")
