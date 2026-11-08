"""Fabricated-only PE32 verifier regression tests, original DLL not required."""
import importlib.util
import struct
import sys
from pathlib import Path
TOOLS=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location('ordinary_bone_audit',
    TOOLS/'audit_sin_original_ordinary_bone.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def fake():
    b=bytearray(0x400)
    b[:2]=b'MZ'
    struct.pack_into('<I',b,0x3c,0x80)
    b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',b,0x86,1)
    struct.pack_into('<H',b,0x94,0xe0)
    struct.pack_into('<H',b,0x98,0x10b)
    struct.pack_into('<I',b,0x98+28,0x22000000)
    struct.pack_into('<IIII',b,0x98+0xe0+8,0x40,0x1000,0x40,0x280)
    b[0x280:0x285]=b'\xe8\xfb\x0f\x00\x00'
    return b
def rejects(action):
    try:action()
    except ValueError:return
    raise AssertionError('malformed x86 original witness should be rejected')
def main():
    p=audit.PE32(fake())
    assert audit.branch(p,0x22001000)==0x22002000
    rejects(lambda:audit.branch(p,0x22001001))
    rejects(lambda:audit.read(p,0x2200103f,2))
    rejects(lambda:audit.verify(fake()))
    b=fake();b[0]=0;rejects(lambda:audit.PE32(b))
    b=fake();b[0x98]=0x0c;rejects(lambda:audit.PE32(b))
    assert len(audit.FUNCTIONS)==3
    assert len(audit.CALLS)==4
    assert len(audit.ANCHORS)==14
if __name__=='__main__':main();print('Synthetic original SiN ordinary bone audit PASS')
