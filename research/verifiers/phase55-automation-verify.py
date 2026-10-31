#!/usr/bin/env python3
"""Optional 14 NEW fixed instruction signature checks against original SiN DLL.
Requires exact user-owned archive, not run when the archive is unavailable.
"""
import argparse
from pathlib import Path
from phase54_verify import load
EXTRA={
 0x222af280:'8b 86 e4 04 00 00',
 0x222af289:'e8 e2 b4 ff ff',
 0x222af28e:'85 c0',
 0x222af290:'7f 07',
 0x222af6b7:'ff 92 38 04 00 00',
 0x222af6db:'2b c3',
 0x222af73f:'89 96 ec 04 00 00',
 0x222af74f:'e8 1c af ff ff',
 0x222af758:'ff 92 3c 04 00 00',
 0x222af77c:'2b c5',
 0x222af7e0:'89 96 f0 04 00 00',
 0x222af7f0:'e8 7b ae ff ff',
 0x222af7f5:'80 be 08 05 00 00 00',
 0x222af800:'c6 86 ac 04 00 00 00',
}
def verify(path):
    dlls,_=load(path,'sin')
    s=dlls['server.dll']
    for va,hexdump in EXTRA.items():
        expected=bytes.fromhex(hexdump)
        assert s.read(va,len(expected))==expected, f'Original binary mismatch at {va:#x}'
    return len(EXTRA)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('original_zip',type=Path)
    a=p.parse_args()
    print(f'PASS Phase55 original SiN binary: {verify(a.original_zip)} extra instruction signatures')
