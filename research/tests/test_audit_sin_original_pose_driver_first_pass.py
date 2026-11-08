"""Synthetic x86 PE32 witness-test; no copyrighted original data required."""
import importlib.util
import struct
import sys
from pathlib import Path
TOOLS=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location('pose_audit',
    TOOLS/'audit_sin_original_pose_driver_first_pass.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
def fake():
    b=bytearray(0x400)
    b[:2]=b'MZ';struct.pack_into('<I',b,0x3c,0x80)
    b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',b,0x86,1);struct.pack_into('<H',b,0x94,0xe0)
    struct.pack_into('<H',b,0x98,0x10b)
    struct.pack_into('<I',b,0x98+28,0x22000000)
    struct.pack_into('<IIII',b,0x98+0xe0+8,0x40,0x1000,0x40,0x280)
    b[0x280:0x285]=b'\xe8\xfb\x0f\x00\x00'
    return b
def reject(fn):
    try:fn()
    except ValueError:return
    raise AssertionError('must reject a malformed binary or invalid witness')
def test():
    pe=audit.PE32(fake())
    assert audit.base(pe)==0x22000000
    assert audit.call_target(pe,0x22001000)==0x22002000
    reject(lambda:audit.call_target(pe,0x22001001))
    reject(lambda:audit.read(pe,0x2200103f,2))
    reject(lambda:audit.verify(fake()))
    b=fake();b[0]=0;reject(lambda:audit.PE32(b))
    b=fake();b[0x98]=0x0c;reject(lambda:audit.PE32(b))
    assert len(audit.FUNCTIONS)==4
    assert len(audit.CALLS)==16
    assert len(audit.ANCHORS)==23
    assert len(audit.JUMP_TABLE)==4
if __name__=='__main__':
    test();print('Synthetic SiN pose first-loop binary audit PASS')
