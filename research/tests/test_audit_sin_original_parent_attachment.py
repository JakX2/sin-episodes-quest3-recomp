"""Original-free PE32 mutation checks for attachment selector audit."""
import importlib.util
from pathlib import Path
import struct
import sys
TOOLS=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location(
    'parent_attachment',TOOLS/'audit_sin_original_parent_attachment.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def fake_pe():
    b=bytearray(0x400)
    b[:2]=b'MZ'
    struct.pack_into('<I',b,0x3c,0x80)
    b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',b,0x86,1)
    struct.pack_into('<H',b,0x94,0xE0)
    struct.pack_into('<H',b,0x98,0x10B)
    struct.pack_into('<I',b,0x98+28,0x22000000)
    struct.pack_into('<IIII',b,0x98+0xE0+8,0x40,0x1000,0x40,0x280)
    b[0x280:0x285]=b'\xE8\xFB\x0F\x00\x00'
    return b
def rejects(f):
    try:f()
    except ValueError:return
    raise AssertionError('unexpected PE32 accept')
def test():
    pe=audit.PE32(fake_pe())
    assert pe.branch_target(0x22001000,0xE8)==0x22002000
    rejects(lambda:pe.branch_target(0x22001000,0xE9))
    rejects(lambda:pe.read(0x2200103f,2))
    rejects(lambda:audit.verify(fake_pe()))
    bad=fake_pe();bad[0]=0;rejects(lambda:audit.PE32(bad))
    bad=fake_pe();bad[0x98]=0x0c
    rejects(lambda:audit.PE32(bad))
    bad=fake_pe();struct.pack_into('<I',bad,0x98+0xE0+20,0xffffffff)
    rejects(lambda:audit.PE32(bad))
    assert len(audit.FUNCTIONS)==4
    assert len(audit.CALLS)==9
    assert len(audit.ANCHORS)==16
if __name__=='__main__':
    test()
    print('Synthetic original parent attachment PE32 audit PASS')
