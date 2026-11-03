#!/usr/bin/env python3
"""Real shared-library C ABI test. No SourceVR/SiN proprietary files needed."""
from __future__ import annotations
import ctypes as c
import hashlib
import os
from pathlib import Path
import sys
import threading
import unittest

SO = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else None
if SO is None or not SO.is_file():
    raise RuntimeError('Usage: test_phase100_ctypes.py /path/to/libsin_legacy_bridge_abi.so')
lib = c.CDLL(str(SO))
P = c.c_void_p
U8 = c.POINTER(c.c_uint8)
U32 = c.c_uint32
I32 = c.c_int32
I64 = c.c_int64
lib.sin100_abi_version.argtypes = []
lib.sin100_abi_version.restype = U32
lib.sin100_create.argtypes = [U32, c.POINTER(P)]
lib.sin100_create.restype = c.c_int
lib.sin100_destroy.argtypes = [P]
lib.sin100_destroy.restype = None
lib.sin100_install_keymap_le.argtypes = [P, U8, U32]
lib.sin100_install_keymap_le.restype = c.c_int
lib.sin100_translate_key.argtypes = [P, I32, c.POINTER(U32)]
lib.sin100_translate_key.restype = c.c_int
lib.sin100_bind_sound.argtypes = [P, I32, I32, I64, U32, P, U32, P, U32]
lib.sin100_bind_sound.restype = c.c_int
lib.sin100_old_to_native.argtypes = [P, I32, c.POINTER(U32)]
lib.sin100_old_to_native.restype = c.c_int
lib.sin100_native_to_old.argtypes = [P, U32, c.POINTER(U32)]
lib.sin100_native_to_old.restype = c.c_int
lib.sin100_copy_sound_name.argtypes = [P, I32, P, U32, c.POINTER(U32)]
lib.sin100_copy_sound_name.restype = c.c_int
lib.sin100_sound_count.argtypes = [P, c.POINTER(U32)]
lib.sin100_sound_count.restype = c.c_int
lib.sin100_reset.argtypes = [P]
lib.sin100_reset.restype = c.c_int

class SharedLibraryTests(unittest.TestCase):
    def setUp(self):
        self.ctx = P()
        self.assertEqual(lib.sin100_create(0x10000,c.byref(self.ctx)),0)
        self.assertTrue(self.ctx.value)
    def tearDown(self):
        lib.sin100_destroy(self.ctx)
    def register(self,old,native,name='step',count=200000,valid=1):
        b=name.encode('utf-8')
        return lib.sin100_bind_sound(self.ctx,old,count,native,valid,c.c_char_p(b),len(b),c.c_char_p(b),len(b))
    def test_version_and_invalid_creation(self):
        self.assertEqual(lib.sin100_abi_version(),0x10000)
        x=P(12345)
        self.assertEqual(lib.sin100_create(0x20000,c.byref(x)),-2)
        self.assertIsNone(x.value)
    def test_invalid_hash_fails_closed(self):
        data=(c.c_uint8*1024)()
        self.assertEqual(lib.sin100_install_keymap_le(self.ctx,data,1024),-4)
        self.assertEqual(lib.sin100_install_keymap_le(self.ctx,data,5),-1)
        out=U32(777)
        self.assertEqual(lib.sin100_translate_key(self.ctx,48,c.byref(out)),-3)
        self.assertEqual(out.value,0)
    def test_private_sha_verified_keymap_when_available(self):
        path=os.environ.get('SIN100_PRIVATE_KEYMAP_BYTES')
        if not path:self.skipTest('public CI does not possess owner-private key map')
        raw=Path(path).read_bytes()
        self.assertEqual(len(raw),1024)
        self.assertEqual(hashlib.sha256(raw).hexdigest(),
                         '6c743e453ce5d6b91214fa2db2fa6b935d7b1ca886c34ebda9538f1ab80a10b0')
        src=(c.c_uint8*1024).from_buffer_copy(raw)
        self.assertEqual(lib.sin100_install_keymap_le(self.ctx,src,1024),0)
        for key,expected in [(48,1),(-1,0),(256,0),(49,2),(8,66),(9,67)]:
            dest=U32(999)
            self.assertEqual(lib.sin100_translate_key(self.ctx,key,c.byref(dest)),0)
            self.assertEqual(dest.value,expected)
        src[0]^=1
        self.assertEqual(lib.sin100_install_keymap_le(self.ctx,src,1024),-4)
        dest=U32(777)
        self.assertEqual(lib.sin100_translate_key(self.ctx,48,c.byref(dest)),-3)
    def test_large_legacy_id_without_truncation(self):
        self.assertEqual(self.register(70000,44),0)
        out=U32(0)
        self.assertEqual(lib.sin100_old_to_native(self.ctx,70000,c.byref(out)),0)
        self.assertEqual(out.value,44)
        self.assertEqual(lib.sin100_old_to_native(self.ctx,4464,c.byref(out)),-10)
        self.assertEqual(out.value,0xffffffff)
        self.assertEqual(lib.sin100_native_to_old(self.ctx,44,c.byref(out)),0)
        self.assertEqual(out.value,70000)
    def test_all_token_invalid_paths(self):
        self.assertEqual(self.register(-1,1),-6)
        self.assertEqual(self.register(100,1,count=100),-6)
        self.assertEqual(self.register(4,-1),-13)
        self.assertEqual(self.register(4,65535),-13)
        self.assertEqual(self.register(4,65536),-13)
        self.assertEqual(self.register(4,1,valid=0),-7)
        self.assertEqual(self.register(4,1,valid=2),-1)
    def test_roundtrip_collision_and_idempotency(self):
        self.assertEqual(self.register(1,2,'one'),0)
        self.assertEqual(self.register(1,2,'one'),1)
        self.assertEqual(self.register(1,3,'one'),-9)
        self.assertEqual(self.register(2,2,'two'),-9)
        self.assertEqual(self.register(2,3,'one'),-9)
        self.assertEqual(self.register(2,3,'two'),0)
    def test_roundtrip_mismatch(self):
        old,native=b'Step',b'step'
        self.assertEqual(lib.sin100_bind_sound(self.ctx,5,99,5,1,
              c.c_char_p(old),len(old),c.c_char_p(native),len(native)),-8)
        self.assertEqual(self.register(5,5,''),-8)
    def test_embedded_nul_rejected(self):
        raw=b'a\0b'
        self.assertEqual(lib.sin100_bind_sound(self.ctx,5,99,5,1,
              c.c_char_p(raw),len(raw),c.c_char_p(raw),len(raw)),-1)
    def test_name_copy_two_pass_no_partial_write(self):
        self.assertEqual(self.register(5,5,'name!'),0)
        needed=U32()
        self.assertEqual(lib.sin100_copy_sound_name(self.ctx,5,None,0,c.byref(needed)),-11)
        self.assertEqual(needed.value,6)
        small=c.create_string_buffer(b'XYZ',4)
        self.assertEqual(lib.sin100_copy_sound_name(self.ctx,5,small,4,c.byref(needed)),-11)
        self.assertEqual(small.value,b'XYZ')
        dest=c.create_string_buffer(6)
        self.assertEqual(lib.sin100_copy_sound_name(self.ctx,5,dest,6,c.byref(needed)),0)
        self.assertEqual(dest.value,b'name!')
    def test_many_insertions_and_reset(self):
        for index in range(1,201):
            self.assertEqual(self.register(10000+index,index,f'sound_{index}'),0)
        count=U32()
        self.assertEqual(lib.sin100_sound_count(self.ctx,c.byref(count)),0)
        self.assertEqual(count.value,200)
        self.assertEqual(lib.sin100_reset(self.ctx),0)
        self.assertEqual(lib.sin100_sound_count(self.ctx,c.byref(count)),0)
        self.assertEqual(count.value,0)
    def test_context_isolation(self):
        other=P()
        self.assertEqual(lib.sin100_create(0x10000,c.byref(other)),0)
        try:
            self.assertEqual(self.register(5,7),0)
            out=U32()
            self.assertEqual(lib.sin100_old_to_native(other,5,c.byref(out)),-10)
        finally:
            lib.sin100_destroy(other)
    def test_parallel_writers_have_serialized_state(self):
        results=[]
        def worker(i):
            status=self.register(9000+i,i,f'parallel_{i}')
            results.append(status)
        threads=[threading.Thread(target=worker,args=(i,)) for i in range(1,101)]
        for t in threads:t.start()
        for t in threads:t.join()
        self.assertEqual(len(results),100)
        self.assertEqual(set(results),{0})
        count=U32()
        self.assertEqual(lib.sin100_sound_count(self.ctx,c.byref(count)),0)
        self.assertEqual(count.value,100)

if __name__=='__main__':
    unittest.main(argv=[sys.argv[0]],verbosity=2)
