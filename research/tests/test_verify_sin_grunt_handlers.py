"""Synthetic PE32 instruction/registration fixture, no original DLL bytes."""
import hashlib
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import verify_sin_grunt_handlers as audit


def fabricated_image():
    # Self-authored, minimal three-section PE covering arbitrary pinned VAs.
    # No original game image, native instruction stream or assets copied here.
    starts = ((0x00357000, 0x12000), (0x00485000, 0x10000), (0x00535000, 0x2000))
    raw_offsets = (0x400, 0x12400, 0x22400)
    total = 0x24800
    image = bytearray(total)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 0x3c, 0x80)
    image[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<HHIIIHH", image, 0x84, 0x14c, 3, 0, 0, 0, 224, 0)
    struct.pack_into("<H", image, 0x98, 0x10b)
    struct.pack_into("<I", image, 0x98+28, 0x22000000)
    for i, ((rva,size), off) in enumerate(zip(starts,raw_offsets)):
        row = 0x98+224 + i*40
        struct.pack_into("<IIII", image,row+8,size,rva,size,off)

    def location(va):
        rva=va-0x22000000
        for (start,size),off in zip(starts,raw_offsets):
            if start<=rva<start+size:
                return off+(rva-start)
        raise ValueError(hex(va))
    for name,(desc,name_va,handler) in audit.REGISTERED_HANDLERS.items():
        encoded=name.encode()+b'\0'
        image[location(name_va):location(name_va)+len(encoded)]=encoded
        struct.pack_into("<I",image,location(desc),name_va)
        struct.pack_into("<I",image,location(desc+8),handler)
    for va,signature in audit.INSTRUCTIONS.items():
        code=bytes.fromhex(signature)
        image[location(va):location(va)+len(code)]=code
    return bytes(image),location


class GruntHandlerVerifierTests(unittest.TestCase):
    def check(self,raw,*,signatures=None,registry=None):
        return audit.verify(
            raw,sha256=hashlib.sha256(raw).hexdigest(),
            signatures=signatures,registrations=registry)

    def test_all_five_synthetic_handlers(self):
        raw,_=fabricated_image()
        result=self.check(raw)
        self.assertEqual(len(result['input_handler_registry_verified']),5)
        self.assertEqual(result['instruction_signatures_verified'],len(audit.INSTRUCTIONS))
        self.assertFalse(result['original_ai_schedule_reconstructed'])

    def test_real_hash_rejects_fixture(self):
        raw,_=fabricated_image()
        with self.assertRaisesRegex(audit.GruntHandlerError,"SHA-256 mismatch"):
            audit.verify(raw)

    def test_mutated_function_pointer(self):
        raw,offset=fabricated_image()
        tampered=bytearray(raw)
        at=offset(0x22535528)
        struct.pack_into("<I",tampered,at,0x22357e30)
        with self.assertRaisesRegex(audit.GruntHandlerError,"handler pointer mismatch"):
            self.check(bytes(tampered))

    def test_mutated_registered_name(self):
        raw,offset=fabricated_image()
        tampered=bytearray(raw)
        tampered[offset(0x22490f20)]=ord("X")
        with self.assertRaisesRegex(audit.GruntHandlerError,"input name mismatch"):
            self.check(bytes(tampered))

    def test_mutated_lookoff_flag_instruction(self):
        raw,offset=fabricated_image()
        tampered=bytearray(raw)
        tampered[offset(0x2235f268)+1]=0x02
        with self.assertRaisesRegex(audit.GruntHandlerError,"instruction mismatch"):
            self.check(bytes(tampered))

    def test_mutated_typed_variant_branch(self):
        raw,offset=fabricated_image()
        tampered=bytearray(raw)
        tampered[offset(0x22357ea4)+3]=0x04
        with self.assertRaisesRegex(audit.GruntHandlerError,"instruction mismatch"):
            self.check(bytes(tampered))

    def test_mutated_patrol_flag(self):
        raw,offset=fabricated_image()
        tampered=bytearray(raw)
        tampered[offset(0x22357e30)+6]=0x02
        with self.assertRaisesRegex(audit.GruntHandlerError,"instruction mismatch"):
            self.check(bytes(tampered))

    def test_mutated_grenade_aux_reset(self):
        raw,offset=fabricated_image()
        tampered=bytearray(raw)
        tampered[offset(0x22357ead)+2]=0x74
        with self.assertRaisesRegex(audit.GruntHandlerError,"instruction mismatch"):
            self.check(bytes(tampered))


if __name__ == "__main__":
    unittest.main()
