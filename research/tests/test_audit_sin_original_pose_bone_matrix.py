"""Synthetic-only PE32 audit regression; never embeds original game bytes."""
import importlib.util
import struct
import sys
from pathlib import Path
TOOLS=Path(__file__).resolve().parents[1]/'tools'
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location('recursive_bone_audit',
    TOOLS/'audit_sin_original_pose_bone_matrix.py')
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def synthetic():
    data=bytearray(0x400)
    data[:2]=b'MZ';struct.pack_into('<I',data,0x3C,0x80)
    data[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<H',data,0x86,1)
    struct.pack_into('<H',data,0x94,0xE0)
    struct.pack_into('<H',data,0x98,0x10B)
    struct.pack_into('<I',data,0x98+28,0x22000000)
    struct.pack_into('<IIII',data,0x98+0xE0+8,0x40,0x1000,0x40,0x280)
    data[0x280:0x285]=b'\xE8\xFB\x0F\x00\x00'
    return data
def rejects(f):
    try:f()
    except ValueError:return
    raise AssertionError('expected rejection')
def test():
    pe=audit.PE32(synthetic())
    assert audit.target(pe,0x22001000)==0x22002000
    rejects(lambda:audit.target(pe,0x22001001))
    rejects(lambda:audit.read(pe,0x2200103f,2))
    rejects(lambda:audit.verify(synthetic()))
    broken=synthetic();broken[0]=0
    rejects(lambda:audit.PE32(broken))
    assert len(audit.REGIONS)==5
    assert len(audit.CALLS)==8
    assert len(audit.ANCHORS)==15
if __name__=='__main__':
    test();print('Synthetic recursive pose-bone audit PASS')
