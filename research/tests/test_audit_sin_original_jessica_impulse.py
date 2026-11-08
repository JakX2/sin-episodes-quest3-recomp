"""Synthetic PE32 regression tests; no original game binaries required."""
import importlib.util
import pathlib
import struct
import sys

ROOT=pathlib.Path(__file__).resolve().parents[1]
TOOLS=ROOT/"tools"
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location("jessica_impulse_audit",
    TOOLS/"audit_sin_original_jessica_impulse.py")
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def fake_pe():
    data=bytearray(0x400)
    data[0:2]=b"MZ"
    struct.pack_into("<I",data,0x3C,0x80)
    data[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<H",data,0x86,1)
    struct.pack_into("<H",data,0x94,0xE0)
    struct.pack_into("<H",data,0x98,0x10B)
    struct.pack_into("<I",data,0x98+28,0x22000000)
    struct.pack_into("<IIII",data,0x98+0xE0+8,
                     0x40,0x1000,0x40,0x280)
    data[0x280:0x285]=b"\xE8\xFB\x0F\x00\x00"
    return data

def raises_value_error(f):
    try:
        f()
    except ValueError:
        return
    raise AssertionError("expected malformed or mismatching data to fail")

def test():
    pe=audit.PE32(fake_pe())
    assert pe.read(0x22001000,5)==b"\xE8\xFB\x0F\x00\x00"
    assert pe.branch_target(0x22001000,0xE8)==0x22002000
    raises_value_error(lambda:pe.read(0x2200103F,2))
    raises_value_error(lambda:pe.branch_target(0x22001000,0xE9))
    raises_value_error(lambda:audit.verify(fake_pe()))
    bad=fake_pe()
    bad[0]=0
    raises_value_error(lambda:audit.PE32(bad))
    bad=fake_pe()
    bad[0x98]=0x0C
    raises_value_error(lambda:audit.PE32(bad))
    bad=fake_pe()
    struct.pack_into("<I",bad,0x98+0xE0+20,0xFFFFFFFF)
    raises_value_error(lambda:audit.PE32(bad))
    assert audit.START==0x2214BA20
    assert audit.END-audit.START==177
    assert len(audit.CALLS)==3

if __name__=="__main__":
    test()
    print("Jessica original impulse audit synthetic tests PASS")
