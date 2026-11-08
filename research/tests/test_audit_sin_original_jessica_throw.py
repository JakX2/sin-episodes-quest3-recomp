"""Original-free PE32 parser and malformed-input regression for Jessica audit."""
import importlib.util
import pathlib
import struct

script = pathlib.Path(__file__).resolve().parents[1]/"tools"/"audit_sin_original_jessica_throw.py"
spec = importlib.util.spec_from_file_location("jessica_audit",script)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def fake_pe():
    b=bytearray(0x300)
    b[:2]=b"MZ"
    struct.pack_into("<I",b,0x3c,0x80)
    b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<H",b,0x86,1)
    struct.pack_into("<H",b,0x94,0xe0)
    struct.pack_into("<H",b,0x98,0x10b)
    struct.pack_into("<I",b,0x98+28,0x22000000)
    at=0x98+0xe0
    struct.pack_into("<IIII",b,at+8,0x20,0x1000,0x20,0x280)
    b[0x280:0x285]=b"\xe8\xfb\x0f\x00\x00"
    return b

def rejects(data):
    try:
        audit.PE32(data)
    except ValueError:
        return
    raise AssertionError("malformed PE32 accepted")

def test_synthetic():
    b=fake_pe()
    pe=audit.PE32(b)
    assert pe.read(0x22001000,5)==b"\xe8\xfb\x0f\x00\x00"
    assert pe.branch_target(0x22001000,0xE8)==0x22002000
    for va,n in [(0x22000000,1),(0x2200101f,2),(0x22001000,33)]:
        try: pe.read(va,n)
        except ValueError: pass
        else: raise AssertionError("out of range read accepted")
    try: pe.branch_target(0x22001000,0xE9)
    except ValueError: pass
    else: raise AssertionError("invalid opcode accepted")
    x=fake_pe();x[0]=0;rejects(x)
    x=fake_pe();x[0x80]=0;rejects(x)
    x=fake_pe();struct.pack_into("<H",x,0x98,0x20b);rejects(x)
    x=fake_pe();struct.pack_into("<I",x,0x98+0xe0+20,0xffffffff);rejects(x)
    rejects(fake_pe()[:0x180])

if __name__=="__main__":
    test_synthetic()
    print("Original-free Jessica PE verifier tests PASS")
