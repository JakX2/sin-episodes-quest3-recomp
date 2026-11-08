"""Synthetic PE32 witness corruption tests; no original SiN bytes bundled."""
import importlib.util
import struct
import sys
from pathlib import Path
TOOLS=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location('second_pose_audit',
    TOOLS/'audit_sin_original_pose_second_pass_state.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def synthetic():
    b=bytearray(0x400)
    b[:2]=b'MZ';struct.pack_into('<I',b,0x3C,0x80)
    b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',b,0x86,1)
    struct.pack_into('<H',b,0x94,0xe0)
    struct.pack_into('<H',b,0x98,0x10b)
    struct.pack_into('<I',b,0x98+28,0x22000000)
    struct.pack_into('<IIII',b,0x98+0xe0+8,0x40,0x1000,0x40,0x280)
    b[0x280:0x285]=b'\xE8\xFB\x0F\x00\x00'
    return b
def rejects(fn):
    try:fn()
    except ValueError:return
    raise AssertionError('invalid original or synthetic input should be rejected')
def test():
    b=synthetic();pe=audit.PE32(b)
    assert audit.call_target(pe,0x22001000)==0x22002000
    rejects(lambda:audit.call_target(pe,0x22001001))
    rejects(lambda:audit.read(pe,0x2200103f,2))
    rejects(lambda:audit.verify(b))
    mutated=synthetic();mutated[0]=0
    rejects(lambda:audit.PE32(mutated))
    assert len(audit.REGIONS)==5
    assert len(audit.CALLS)==5
    assert len(audit.ANCHORS)==21
    assert len(audit.CONSTANTS)==2
if __name__=='__main__':
    test();print('Synthetic SiN second pose state DLL verifier PASS')
