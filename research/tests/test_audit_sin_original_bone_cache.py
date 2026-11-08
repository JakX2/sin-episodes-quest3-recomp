"""Synthetic-only PE32 mutation and witness-shape tests, no game binary."""
import importlib.util
import struct
import sys
from pathlib import Path
TOOLS=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location('bone_cache_audit',
    TOOLS/'audit_sin_original_bone_cache.py')
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
def rejects(fn):
    try:fn()
    except ValueError:return
    raise AssertionError('must reject bad original or synthetic PE32')
def test():
    p=audit.PE32(fake_pe())
    assert audit.base(p)==0x22000000
    assert audit.call_target(p,0x22001000)==0x22002000
    rejects(lambda:audit.call_target(p,0x22001001))
    rejects(lambda:audit.read(p,0x2200103f,2))
    rejects(lambda:audit.verify(fake_pe()))
    b=fake_pe();b[0]=0;rejects(lambda:audit.PE32(b))
    b=fake_pe();b[0x98]=0x0c;rejects(lambda:audit.PE32(b))
    b=fake_pe();struct.pack_into('<I',b,0x98+0xE0+20,0xffffffff)
    rejects(lambda:audit.PE32(b))
    assert len(audit.FUNCTIONS)==4
    assert len(audit.CALLS)==7
    assert len(audit.ANCHORS)==17
if __name__=='__main__':
    test();print('Synthetic SiN original bone cache audit PASS')
