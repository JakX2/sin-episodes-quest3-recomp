"""Synthetic-only PE32 mutation coverage. Does not bundle any game bytes."""
import importlib.util
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(ROOT))
spec=importlib.util.spec_from_file_location(
    'original_skeletal_setup_audit',ROOT/'audit_sin_original_skeletal_setup.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def fake_pe():
    b=bytearray(0x400);b[:2]=b'MZ'
    struct.pack_into('<I',b,0x3c,0x80)
    b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',b,0x86,1)
    struct.pack_into('<H',b,0x94,0xe0)
    struct.pack_into('<H',b,0x98,0x10b)
    struct.pack_into('<I',b,0x98+28,0x22000000)
    struct.pack_into('<IIII',b,0x98+0xe0+8,0x40,0x1000,0x40,0x280)
    b[0x280:0x285]=b'\xe8\xfb\x0f\x00\x00'
    return b
def rejects(cb):
    try:cb()
    except ValueError:return
    raise AssertionError('bad PE or wrong binary was accepted')
def main():
    p=audit.PE32(fake_pe())
    assert audit.base(p)==0x22000000
    assert audit.call_target(p,0x22001000)==0x22002000
    rejects(lambda:audit.call_target(p,0x22001001))
    rejects(lambda:audit.read(p,0x2200103f,2))
    rejects(lambda:audit.verify(fake_pe()))
    b=fake_pe();b[0]=0;rejects(lambda:audit.PE32(b))
    b=fake_pe();b[0x98]=0x0c;rejects(lambda:audit.PE32(b))
    b=fake_pe();struct.pack_into('<I',b,0x98+0xe0+20,0xffffffff)
    rejects(lambda:audit.PE32(b))
    assert len(audit.FUNCTIONS)==3
    assert len(audit.CALLS)==22
    assert len(audit.ANCHORS)==21
    assert len(audit.VTABLES)==8
    assert audit.VTABLES['CBaseAnimating'][1]!=audit.VTABLES['CNPC_Jessica'][1]
    print('Synthetic original skeletal setup audit PASS')
if __name__=='__main__':main()
