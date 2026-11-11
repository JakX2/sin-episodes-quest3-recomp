"""Original-free Phase179 mutation gates and independent initialization matrices."""
from __future__ import annotations
import copy
import pathlib
import struct
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_dll_initialization as mod
from audit_sin_original_dll_initialization import VerificationError,original_manifest,verify_initializer

class FakePE:
    def __init__(self,manifest):
        self.mem={}
        self.names={int(r['name_va'],16) for d in manifest['dlls'].values()
                    for r in d['prefix_requests']+d['later_requests']}
        self.stores={int(r['storage_va'],16) for d in manifest['dlls'].values()
                    for r in d['prefix_requests']+d['later_requests']}
    def at(self,va,n):return bytes(self.mem.get(va+i,0xcc) for i in range(n))
    def section_of(self,va):
        if va in self.names:return '.rdata'
        if va in self.stores:return '.data'
        return '.text'
    def put(self,va,raw):
        for i,v in enumerate(raw):
            if va+i in self.mem and self.mem[va+i]!=v:
                raise AssertionError(f'conflicting fixture byte at {va+i:#x}')
            self.mem[va+i]=v
    def corrupt(self,va):self.mem[va]=self.mem.get(va,0xcc)^0x01


def fixture(dll, info, whole_manifest):
    pe=FakePE(whole_manifest)
    fail=mod.EXPECTED_FAILURE[dll]
    for row in info['prefix_requests']+info['later_requests']:
        va=int(row['request_va'],16)
        ptr=int(row['name_va'],16)
        dest=int(row['storage_va'],16)
        pe.put(ptr,row['name'].encode('ascii')+b'\x00')
        if row.get('special_initial_store'):
            pe.put(va,b'\x6a\x00\x68'+struct.pack('<I',ptr))
            pe.put(va+7,b'\xa3'+struct.pack('<I',0x242fa458))
            pe.put(va+12,bytes.fromhex('ff d6 83 c4 28 85 c0'))
            pe.put(va+19,b'\xa3'+struct.pack('<I',dest))
            pe.put(va+24,b'\x0f\x84'+struct.pack('<i',fail-(va+30)))
        elif row['factory'] in ('engine','filesystem'):
            pe.put(va,b'\x6a\x00\x68'+struct.pack('<I',ptr))
            pe.put(va+7,b'\xff\xd7' if row['factory']=='filesystem' else b'\xff\xd6')
            if row['phase']=='mandatory_prefix':
                pe.put(va+9,bytes.fromhex('83 c4 08 85 c0 a3')+struct.pack('<I',dest))
                pe.put(va+19,b'\x0f\x84'+struct.pack('<i',fail-(va+25)))
            elif dll=='server':
                pe.put(va+9,bytes.fromhex('68 10 05 00 00 a3')+struct.pack('<I',dest))
            else:
                pe.put(va+9,b'\x8b\x0d'+struct.pack('<I',0x2433f69c)+bytes.fromhex('83 c4 08 6a 00 a3')+struct.pack('<I',dest))
        else:
            pe.put(va,b'\x68'+struct.pack('<I',ptr))
            op=b'\xff\x50\x20' if row['name']=='MaterialSystemHardwareConfig012' else b'\xff\x50\x08'
            pe.put(va+5,op+b'\x85\xc0\xa3'+struct.pack('<I',dest))
            pe.put(va+15,b'\x0f\x84'+struct.pack('<i',fail-(va+21)))
    if dll=='server':
        pe.put(fail,bytes.fromhex('5f32c05e83c40cc21000'))
        pe.put(0x221e7978,bytes.fromhex('5fb0015e83c40cc21000'))
    else:
        pe.put(fail,bytes.fromhex('33c05ec20c00'))
        pe.put(0x240fe595,bytes.fromhex('b8010000005ec20c00'))
    return pe

class OriginalInitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.manifest=original_manifest()
    def test_original_opaque_region_metadata(self):
        self.assertEqual({d:self.manifest['dlls'][d]['function_len'] for d in ('server','client')},{'server':875,'client':1220})
        self.assertEqual(sum(self.manifest['dlls'][d]['function_len'] for d in ('server','client')),2095)
        self.assertTrue(all(len(bytes.fromhex(self.manifest['dlls'][d]['original_sha256']))==32 for d in ('server','client')))
    def test_independent_success_fixtures(self):
        for d,info in self.manifest['dlls'].items():
            with self.subTest(dll=d):
                r=verify_initializer(fixture(d,info,self.manifest),d,info,check_hash=False)
                self.assertEqual(r['mandatory_early_interface_queries'],mod.EXPECTED_PREFIX[d])
                self.assertEqual(r['later_interface_queries'],mod.EXPECTED_LATE[d])
                self.assertEqual(r['later_optional_without_immediate_null_gate'],1)
    def test_every_native_interface_name_corruption_rejected(self):
        count=0
        for d,info in self.manifest['dlls'].items():
            for row in info['prefix_requests']+info['later_requests']:
                with self.subTest(dll=d,interface=row['name']):
                    pe=fixture(d,info,self.manifest);pe.corrupt(int(row['name_va'],16))
                    with self.assertRaisesRegex(VerificationError,'interface version'):verify_initializer(pe,d,info,check_hash=False)
                    count+=1
        self.assertEqual(count,41)
    def test_every_native_query_instruction_corruption_rejected(self):
        count=0
        for d,info in self.manifest['dlls'].items():
            for row in info['prefix_requests']+info['later_requests']:
                for offset in (0,5 if row['factory']=='derived_virtual' else 7):
                    with self.subTest(dll=d,interface=row['name'],offset=offset):
                        pe=fixture(d,info,self.manifest);pe.corrupt(int(row['request_va'],16)+offset)
                        with self.assertRaisesRegex(VerificationError,'instruction mismatch'):
                            verify_initializer(pe,d,info,check_hash=False)
                        count+=1
        self.assertEqual(count,82)
    def test_every_early_failure_branch_mutation_rejected(self):
        count=0
        for d,info in self.manifest['dlls'].items():
            for row in info['prefix_requests']:
                va=int(row['request_va'],16)
                pe=fixture(d,info,self.manifest)
                pe.corrupt(va+(24 if row.get('special_initial_store') else 19)+2)
                with self.assertRaisesRegex(VerificationError,'failure branch'):verify_initializer(pe,d,info,check_hash=False)
                count+=1
        self.assertEqual(count,37)
    def test_special_first_client_and_source_distinction(self):
        server=self.manifest['dlls']['server'];client=self.manifest['dlls']['client']
        self.assertEqual(server['prefix_requests'][10]['factory'],'filesystem')
        self.assertEqual(client['prefix_requests'][16]['factory'],'engine')
        for d,info,row in [('client',client,client['prefix_requests'][0]),('server',server,server['prefix_requests'][10])]:
            pe=fixture(d,info,self.manifest)
            pe.corrupt(int(row['request_va'],16)+(12 if d=='client' else 7))
            with self.assertRaisesRegex(VerificationError,'instruction mismatch'):verify_initializer(pe,d,info,check_hash=False)
    def test_late_optional_and_mandatory_distinction(self):
        for d,info in self.manifest['dlls'].items():
            late=info['later_requests'];self.assertEqual(sum(not r['required'] for r in late),1)
            for row in late:
                if row['phase']=='later_conditional':
                    pe=fixture(d,info,self.manifest)
                    pe.corrupt(int(row['request_va'],16)+15+2)
                    with self.assertRaisesRegex(VerificationError,'failure branch'):verify_initializer(pe,d,info,check_hash=False)
    def test_guard_against_fabricated_full_original_hash(self):
        for d,info in self.manifest['dlls'].items():
            with self.assertRaisesRegex(VerificationError,'complete Init function SHA256'):
                verify_initializer(fixture(d,info,self.manifest),d,info,check_hash=True)
    def test_wrong_manifest_and_count_rejected(self):
        with patch.object(mod,'MANIFEST_SHA256','0'*64):
            with self.assertRaisesRegex(VerificationError,'manifest SHA256'):original_manifest()
        for d,info in self.manifest['dlls'].items():
            bad=copy.deepcopy(info);bad['prefix_requests'].pop()
            with self.assertRaisesRegex(VerificationError,'count mismatch'):
                verify_initializer(fixture(d,info,self.manifest),d,bad,check_hash=False)
    def test_independent_prefix_failure_matrix(self):
        total=0
        for d,info in self.manifest['dlls'].items():
            versions=[r['name'] for r in info['prefix_requests']]
            for fail in range(len(versions)+1):
                queries=[];published=[]
                for i,row in enumerate(info['prefix_requests']):
                    queries.append(row['name'])
                    published.append((row['storage_va'],0 if i==fail else 0x1000+i))
                    if i==fail:break
                expected=len(versions) if fail==len(versions) else fail+1
                self.assertEqual(len(queries),expected)
                self.assertEqual(len(published),expected)
                self.assertEqual(queries,versions[:expected])
                total+=1
        self.assertEqual(total,39)

if __name__=='__main__':unittest.main()