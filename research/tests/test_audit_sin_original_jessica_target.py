"""Original-free PE32 negative regression for Jessica world-center audit."""
from pathlib import Path
import importlib.util
import struct
import sys
TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location(
    'jessica_target_audit',TOOLS/'audit_sin_original_jessica_target.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def synthetic_pe():
    out=bytearray(0x400)
    out[:2]=b'MZ'
    struct.pack_into('<I',out,0x3c,0x80)
    out[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',out,0x80+6,1)
    struct.pack_into('<H',out,0x80+20,0xe0)
    opt=0x80+24
    struct.pack_into('<H',out,opt,0x10b)
    struct.pack_into('<I',out,opt+28,0x22000000)
    section=opt+0xe0
    struct.pack_into('<IIII',out,section+8,0x40,0x1000,0x40,0x280)
    out[0x280:0x285]=b'\xe8\xfb\x0f\x00\x00'
    return out

def expect_error(fn):
    try:fn()
    except ValueError:return
    raise AssertionError('must reject corrupted PE or instruction')

def test():
    data=synthetic_pe()
    image=audit.PE32(data)
    assert image.read(0x22001000,5)==b'\xe8\xfb\x0f\x00\x00'
    assert image.branch_target(0x22001000,0xe8)==0x22002000
    assert audit.checked_pointer(image,0x22001000)==0x0ffbe8
    expect_error(lambda:image.branch_target(0x22001000,0xe9))
    expect_error(lambda:image.read(0x2200103f,2))
    expect_error(lambda:audit.verify(data))
    malformed=synthetic_pe();malformed[0]=0
    expect_error(lambda:audit.PE32(malformed))
    malformed=synthetic_pe();malformed[0x98]=0x0c
    expect_error(lambda:audit.PE32(malformed))
    malformed=synthetic_pe();struct.pack_into('<I',malformed,0x98+0xe0+20,0xffffffff)
    expect_error(lambda:audit.PE32(malformed))
    assert len(audit.VTABLES)==3
    assert len(audit.FUNCTION_HASHES)==3
    assert len(audit.ANCHORS)==12
    assert len(audit.CALLS)==3

if __name__=='__main__':
    test()
    print('Synthetic original Jessica target-center PE verifier PASS')
