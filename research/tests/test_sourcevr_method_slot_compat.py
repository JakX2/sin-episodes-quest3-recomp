"""Synthetic original-free SiN/SourceVR 0.5.5 virtual-slot ABI tests."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_sourcevr_method_slot_compat as a

class FakeELF:
    def __init__(self,kind):
        self.kind=kind
        self.memory={}
        base,length=(0x12660,0x2b304) if kind=='sound' else (0x1b700,0x46f9c)
        self.sections={'.text':(0,1,0,base,0,length,0,0,0,0)}
        lib='libsoundemittersystem.so' if kind=='sound' else 'libvgui2.so'
        for va,word in a.AARCH64[lib].items():self.put(va,struct.pack('<I',word))
        for name,table in a.TABLES.items():
            if (name=='sound') != (kind=='sound'):continue
            _,va,count,_=table
            vals=[base+0x1000+4*i for i in range(count)]
            if name=='sound':
                vals[3]=0x12970;vals[6]=0x13380
            self.put(va,struct.pack('<%dQ'%count,*vals))
            self.put(va+count*8,bytes(8))
    def put(self,va,raw):
        for i,b in enumerate(raw):
            if va+i in self.memory and self.memory[va+i]!=b:
                raise AssertionError('overlapping fixture at %#x'%(va+i))
            self.memory[va+i]=b
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.memory[va]^=0x40
def fake_audit():
    sound=FakeELF('sound');vgui=FakeELF('vgui')
    with patch.object(a,'ELF64',side_effect=[sound,vgui]):
        return a.inspect_native(b'mock sound',b'mock vgui',False,False)

class TestSlotABI(unittest.TestCase):
    def test_inventory_47_89_40(self):
        r=fake_audit()
        self.assertEqual([r['verified_complete_native_vtables'][k]['method_count']
             for k in ('sound','input','system')],[47,89,40])
    def test_sound_same_slot_semantics_proven_wrong(self):
        r=fake_audit()
        v=r['sound_emitter_method_mismatches']
        self.assertEqual([x['native_observed_behavior'] for x in v],
                         ['constant_true','decrement_reference_count'])
        self.assertTrue(all(x['compatibility']=='proven_incompatible' for x in v))
    def test_client_six_original_call_indexes(self):
        r=fake_audit()
        self.assertEqual([x['index'] for x in r['original_client_used_vgui_slots']],
                         [55,56,58,61,62,24])
        self.assertTrue(all(not x['method_signatures_verified'] for x in r['original_client_used_vgui_slots']))
    def test_all_aarch64_opcode_mutations(self):
        for lib,sites in a.AARCH64.items():
            for va in sites:
                with self.subTest(lib=lib,va=hex(va)):
                    s=FakeELF('sound');v=FakeELF('vgui')
                    (s if lib=='libsoundemittersystem.so' else v).corrupt(va)
                    with patch.object(a,'ELF64',side_effect=[s,v]):
                        with self.assertRaisesRegex(a.VerificationError,'AArch64 opcode'):
                            a.inspect_native(b'x',b'y',False,False)
    def test_all_pointer_mutations_sha_rejected(self):
        for _,va,count,_ in a.TABLES.values():
            kind='sound' if va==0x41f98 else 'vgui'
            d=next(t for t in a.TABLES.values() if t[1]==va)
            for i in range(count):
                with self.subTest(vtable=hex(va),method=i):
                    f=FakeELF(kind);f.corrupt(va+i*8)
                    with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
                        a.validate_table(f,d,True)
    def test_all_native_table_hashes_pinned(self):
        self.assertEqual(len(a.TABLES),3)
        for _,va,_,_ in a.TABLES.values():
            f=FakeELF('sound' if va==0x41f98 else 'vgui')
            d=next(t for t in a.TABLES.values() if t[1]==va)
            with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
                a.validate_table(f,d,True)
    def test_vtable_boundaries(self):
        for d in a.TABLES.values():
            _,va,count,_=d
            f=FakeELF('sound' if va==0x41f98 else 'vgui')
            f.corrupt(va+count*8)
            with self.assertRaisesRegex(a.VerificationError,'boundary'):
                a.validate_table(f,d,False)
    def test_reject_nonexecutable_method_address(self):
        d=a.TABLES['sound'];f=FakeELF('sound')
        for i,b in enumerate(struct.pack('<Q',0x484000)):
            f.memory[d[1]+i]=b
        with self.assertRaisesRegex(a.VerificationError,'non-executable'):
            a.validate_table(f,d,False)
    def test_original_sha_fingerprints_are_required(self):
        with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
            a.inspect_original(b'fake client',b'fake server')
    def test_sound_method_index_relocation_does_not_succeed(self):
        s=FakeELF('sound');v=FakeELF('vgui')
        s.corrupt(0x41f98+3*8)
        with patch.object(a,'ELF64',side_effect=[s,v]):
            with self.assertRaisesRegex(a.VerificationError,'native slot index'):
                a.inspect_native(b's',b'v',False,False)
    def test_pointer_width_ratio(self):
        for slots in a.VGUI_SLOTS.values():
            for index,offset in slots.items():
                self.assertEqual(index*4,offset)
                self.assertEqual(index*8,offset*2)
    def test_native_factories_and_binary_registrations_are_separate(self):
        self.assertEqual(a.TABLES['sound'][0],'libsoundemittersystem.so')
        self.assertEqual(a.TABLES['input'][0],'libvgui2.so')
        self.assertEqual(sum(map(len,a.AARCH64.values())),27)

if __name__=='__main__':unittest.main()
